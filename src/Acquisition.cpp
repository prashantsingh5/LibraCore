#include "bookmgmt/Acquisition.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <limits>
#include "bookmgmt/Validation.h"

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget,
                                       TaxRates taxRates)
    : catalog_(catalog), budget_(budget), taxRates_(taxRates) {
    setTaxRates(taxRates);
}

void AcquisitionManager::setTaxRates(TaxRates taxRates) {
    if (taxRates.printPercent < 0 || taxRates.electronicPercent < 0) {
        throw std::invalid_argument("tax rates must not be negative");
    }

    taxRates_ = taxRates;
}

void AcquisitionManager::addDepartment(const std::string& name,
                                       Budget& budget) {
    requireId(name, "department name");

    if (departmentBudgets_.count(name) > 0) {
        throw std::invalid_argument("department already registered: " + name);
    }

    departmentBudgets_[name] = &budget;
}

bool AcquisitionManager::hasDepartment(const std::string& name) const {
    return departmentBudgets_.count(name) > 0;
}

Budget& AcquisitionManager::budgetForDepartment(
    const std::string& department) {

    if (department.empty()) {
        return budget_;
    }

    auto it = departmentBudgets_.find(department);

    if (it == departmentBudgets_.end()) {
        throw std::invalid_argument(
            "department not found: " + department);
    }

    return *it->second;
}

const Budget& AcquisitionManager::budgetForDepartment(
    const std::string& department) const {

    if (department.empty()) {
        return budget_;
    }

    auto it = departmentBudgets_.find(department);

    if (it == departmentBudgets_.end()) {
        throw std::invalid_argument(
            "department not found: " + department);
    }

    return *it->second;
}

Money AcquisitionManager::applyTax(const Resource& resource,
                                   Money preTaxCost) const {
    const int percent =
        resource.isDigital() ? taxRates_.electronicPercent
                             : taxRates_.printPercent;

    const Money tax =
        preTaxCost.scaledRatio(percent, 100);

    return preTaxCost + tax;
}

void AcquisitionManager::setConfig(AcquisitionConfig config) {
    if (config.vendorSelection != VendorSelection::Cheapest && config.vendorSelection != VendorSelection::Fastest
        && config.vendorSelection != VendorSelection::HighestRated)
        throw std::invalid_argument("invalid vendor selection mode");
    setTaxRates(config.taxes);
    vendorMode_ = config.vendorSelection;
}
AcquisitionManager::PriceChoice AcquisitionManager::priceForPurchase(
    const Resource& r, int quantity, VendorSelection mode) const {
    if (const auto* v = r.selectVendor(mode)) return {r.costForAtPrice(quantity, v->price), *v};
    return {r.costFor(quantity), std::nullopt};
}
Money AcquisitionManager::quote(const std::string& id, int qty) const { return quote(id, qty, vendorMode_); }
Money AcquisitionManager::quote(const std::string& id, int qty, VendorSelection mode) const {
    const auto& r = catalog_.get(id);
    return applyTax(r, priceForPurchase(r, qty, mode).preTaxCost);
}
PurchaseRecord AcquisitionManager::prepare(const PurchaseRequest& req) const {
    requireId(req.resourceId, "resource id");
    if (!req.department.empty()) requireId(req.department, "department");
    const auto& r = catalog_.get(req.resourceId);
    const auto choice = priceForPurchase(r, req.quantity, req.vendorSelection.value_or(vendorMode_));
    const Money post = applyTax(r, choice.preTaxCost);
    (void)budgetForDepartment(req.department);
    return {0, req.resourceId, r.title(), r.category(), req.quantity, choice.preTaxCost, post,
            true, {}, RecordType::Purchase, 0, req.department,
            choice.offer ? choice.offer->vendor : std::string{}, AcquisitionStatus::Approved,
            choice.offer, std::chrono::system_clock::now()};
}
bool AcquisitionManager::canPurchase(const std::string& id, int qty, std::string* reason) const {
    return canPurchase(id, qty, "", reason);
}
bool AcquisitionManager::canPurchase(const std::string& id, int qty,
                                     const std::string& department, std::string* reason) const {
    try {
        auto rec = prepare({id, qty, department});
        Budget simulated = budgetForDepartment(department);
        simulated.commit(rec.category, id, qty, rec.cost);
        if (qty > std::numeric_limits<int>::max() - catalog_.holdings(id))
            throw std::overflow_error("holdings overflow");
        if (reason) reason->clear();
        return true;
    } catch (const std::bad_alloc&) { throw; }
      catch (const std::exception& e) { if (reason) *reason = e.what(); return false; }
}
void AcquisitionManager::commitRecords(std::vector<PurchaseRecord>& records) {
    if (records.size() > static_cast<std::size_t>(std::numeric_limits<int>::max() - nextOrderNo_))
        throw std::overflow_error("order numbers exhausted");
    // Key by object identity: different department names may share one Budget.
    std::map<Budget*, Budget> budgets;
    std::map<std::string, int> holdings;
    auto stagedHistory = history_;
    auto stagedEvents = catalog_.events_;
    int next = nextOrderNo_;
    for (auto& rec : records) {
        rec.orderNo = next++;
        if (rec.approved) {
            auto* target = &budgetForDepartment(rec.department);
            auto [it, inserted] = budgets.try_emplace(target, *target);
            auto [h, added] = holdings.try_emplace(rec.resourceId, catalog_.holdings(rec.resourceId));
            if (rec.type == RecordType::Cancellation) {
                if (h->second - catalog_.lent(rec.resourceId) < rec.quantity)
                    throw std::invalid_argument("insufficient available holdings to cancel order");
                it->second.refund(rec.category, rec.resourceId, rec.quantity, rec.cost);
                h->second -= rec.quantity;
            } else {
                if (rec.quantity > std::numeric_limits<int>::max() - h->second)
                    throw std::overflow_error("holdings overflow");
                it->second.commit(rec.category, rec.resourceId, rec.quantity, rec.cost);
                h->second += rec.quantity;
            }
            stagedEvents.push_back({rec.type == RecordType::Purchase ? EventType::Purchase : EventType::Cancellation,
                rec.resourceId, rec.quantity, rec.department, rec.timestamp, std::to_string(rec.orderNo)});
        }
        stagedHistory.push_back(rec);
    }
    // Nothing below allocates, prices, or validates.
    for (auto& [target, staged] : budgets) target->swap(staged);
    for (const auto& [id, count] : holdings) catalog_.items_.at(id).holdings = count;
    history_.swap(stagedHistory); catalog_.events_.swap(stagedEvents); nextOrderNo_ = next;
}
const PurchaseRecord& AcquisitionManager::purchase(const std::string& id, int qty) {
    return purchase(id, qty, "");
}
const PurchaseRecord& AcquisitionManager::purchase(const std::string& id, int qty, const std::string& department) {
    return purchase(id, qty, vendorMode_, department);
}
const PurchaseRecord& AcquisitionManager::purchase(const std::string& id, int qty, VendorSelection mode,
                                                  const std::string& department) {
    PurchaseRequest req{id, qty, department}; req.vendorSelection = mode;
    std::vector<PurchaseRecord> records{prepare(req)};
    commitRecords(records);
    return history_.back();
}
AcquisitionStatus AcquisitionManager::orderStatus(int orderNo) const {
    if (orderNo <= 0) throw std::invalid_argument("order number must be positive");
    for (const auto& rec : history_) if (rec.orderNo == orderNo) {
        for (const auto& other : history_)
            if (other.type == RecordType::Cancellation && other.relatedOrderNo == orderNo)
                return AcquisitionStatus::Cancelled;
        return rec.status;
    }
    throw std::invalid_argument("order not found");
}
const PurchaseRecord& AcquisitionManager::cancel(int orderNo) {
    if (orderStatus(orderNo) != AcquisitionStatus::Approved)
        throw std::invalid_argument("only an active approved purchase can be cancelled");
    PurchaseRecord cancellation{};
    for (const auto& rec : history_) if (rec.orderNo == orderNo) cancellation = rec;
    cancellation.type = RecordType::Cancellation;
    cancellation.relatedOrderNo = orderNo;
    cancellation.status = AcquisitionStatus::Cancelled;
    cancellation.timestamp = std::chrono::system_clock::now();
    std::vector<PurchaseRecord> records{cancellation};
    commitRecords(records);
    return history_.back();
}
std::vector<PurchaseRecord> AcquisitionManager::processBatch(const std::vector<PurchaseRequest>& reqs) {
    return processBatch(reqs, false);
}
std::vector<PurchaseRecord> AcquisitionManager::processBatch(const std::vector<PurchaseRequest>& reqs, bool atomic) {
    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());
    if (!atomic) {
        for (const auto& req : reqs) {
            auto one = processBatch({req}, true);
            results.push_back(std::move(one.front()));
        }
        return results;
    }
    std::map<const Budget*, Budget> budgets;
    std::map<std::string, int> holdings;
    bool failed = false;
    for (const auto& req : reqs) {
        const auto* r = catalog_.find(req.resourceId);
        PurchaseRecord rec{0, req.resourceId, r ? r->title() : "(unknown)",
            r ? r->category() : ResourceCategory::Book, req.quantity,
            Money::fromMinor(0, budget_.total().currency()), Money::fromMinor(0, budget_.total().currency()),
            false, {}, RecordType::Purchase, 0, req.department, {}, AcquisitionStatus::Rejected,
            std::nullopt, std::chrono::system_clock::now()};
        try {
            rec = prepare(req);
            const auto* target = &budgetForDepartment(req.department);
            auto [it, inserted] = budgets.try_emplace(target, *target);
            auto [h, added] = holdings.try_emplace(req.resourceId, catalog_.holdings(req.resourceId));
            if (req.quantity > std::numeric_limits<int>::max() - h->second)
                throw std::overflow_error("holdings overflow");
            it->second.commit(rec.category, req.resourceId, req.quantity, rec.cost);
            h->second += req.quantity;
        } catch (const std::bad_alloc&) { throw; }
          catch (const std::exception& e) { rec.reason = e.what(); failed = true; }
        results.push_back(std::move(rec));
    }
    if (failed) for (auto& rec : results) {
        rec.approved = false; rec.status = AcquisitionStatus::Rejected;
        if (rec.reason.empty()) rec.reason = "batch rejected: another request in the all-or-nothing batch failed";
    }
    commitRecords(results);
    return results;
}
PurchaseRecord AcquisitionManager::submit(PurchaseRequest& request) {
    if (request.status != AcquisitionStatus::Requested || request.orderNo != 0)
        throw std::invalid_argument("request already processed");
    std::vector<PurchaseRecord> records;
    try {
        auto rec = prepare(request);
        auto simulated = budgetForDepartment(request.department);
        simulated.commit(rec.category, rec.resourceId, rec.quantity, rec.cost);
        if (rec.quantity > std::numeric_limits<int>::max() - catalog_.holdings(rec.resourceId))
            throw std::overflow_error("holdings overflow");
        records.push_back(std::move(rec));
    } catch (const std::bad_alloc&) { throw; }
      catch (const std::exception& e) {
        const auto* r = catalog_.find(request.resourceId);
        records.push_back({0, request.resourceId, r ? r->title() : "(unknown)",
            r ? r->category() : ResourceCategory::Book, request.quantity,
            Money::fromMinor(0, budget_.total().currency()), Money::fromMinor(0, budget_.total().currency()),
            false, e.what(), RecordType::Purchase, 0, request.department, {},
            AcquisitionStatus::Rejected, std::nullopt, std::chrono::system_clock::now()});
    }
    std::string reason = records.front().reason;
    commitRecords(records);
    request.reason.swap(reason);
    request.status = records.front().status; request.orderNo = records.front().orderNo;
    return std::move(records.front());
}

AcquisitionSummary AcquisitionManager::summary() const {
    AcquisitionSummary s; s.totalPreTax = totalPreTax(); s.totalPostTax = totalSpent();
    for (const auto& r : history_) {
        if (r.status == AcquisitionStatus::Cancelled) ++s.cancelled;
        else if (r.status == AcquisitionStatus::Approved) ++s.approved;
        else if (r.status == AcquisitionStatus::Rejected) ++s.rejected;
    }
    return s;
}
std::vector<DepartmentSummary> AcquisitionManager::departmentSummaries() const {
    std::vector<DepartmentSummary> out{{"", budget_.total(), budget_.spent(), budget_.remaining()}};
    for (const auto& [name, b] : departmentBudgets_)
        out.push_back({name, b->total(), b->spent(), b->remaining()});
    return out;
}

Money AcquisitionManager::totalSpent() const {
    bool initialized = false;
    Money sum = Money::fromMinor(
        0,
        budget_.total().currency());

    for (const auto& rec : history_) {
        if (!rec.approved) {
            continue;
        }

        if (!initialized) {
            sum = Money::fromMinor(
                0,
                rec.cost.currency());

            initialized = true;
        }

        if (rec.type == RecordType::Purchase) {
            sum += rec.cost;
        } else if (rec.type == RecordType::Cancellation) {
            sum -= rec.cost;
        }
    }

    return sum;
}

Money AcquisitionManager::totalPreTax() const {
    bool initialized = false;
    Money sum = Money::fromMinor(
        0,
        budget_.total().currency());

    for (const auto& rec : history_) {
        if (!rec.approved) {
            continue;
        }

        if (!initialized) {
            sum = Money::fromMinor(
                0,
                rec.preTaxCost.currency());

            initialized = true;
        }

        if (rec.type == RecordType::Purchase) {
            sum += rec.preTaxCost;
        } else if (rec.type == RecordType::Cancellation) {
            sum -= rec.preTaxCost;
        }
    }

    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " records)\n";

    for (const auto& rec : history_) {
        os << "  #" << std::setw(3) << std::left << rec.orderNo << " ";

        if (rec.type == RecordType::Cancellation) {
            os << "CANCELLED";
        } else {
            os << (rec.approved ? "APPROVED " : "REJECTED ");
        }

        os << "  "
           << std::setw(6) << rec.resourceId
           << " x" << std::setw(3) << rec.quantity
           << " pre-tax " << std::setw(10) << std::right
           << rec.preTaxCost.toString()
           << "  post-tax " << std::setw(10)
           << rec.cost.toString()
           << std::left << "  "
           << rec.title;
           if (!rec.department.empty()) {
                os << "  [department: "
                << rec.department << "]";
            }

            if (!rec.vendor.empty()) {
                os << "  [vendor: "
                << rec.vendor << "]";
            }

        if (rec.type == RecordType::Cancellation) {
            os << "\n        cancels order #"
               << rec.relatedOrderNo;
        } else if (!rec.approved) {
            os << "\n        reason: " << rec.reason;
        }

        os << "\n";
    }

    os << "Total pre-tax: " << totalPreTax() << "\n";
    os << "Total post-tax: " << totalSpent() << "\n";
}

}  // namespace bookmgmt
