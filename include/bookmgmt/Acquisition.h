#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing the
// Budget's quotas, updating Catalog holdings and keeping an order history.

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"

namespace bookmgmt {

struct PersistenceAccess; // V2 adapter: trusted relational state restoration only.

enum class AcquisitionStatus { Requested, Approved, Rejected, Cancelled };

struct PurchaseRequest {
    std::string resourceId;
    int quantity = 0;  // copies for print, seats for electronic
    std::string department = {};  // empty means the default/legacy budget
    AcquisitionStatus status = AcquisitionStatus::Requested;
    std::string reason = {};
    int orderNo = 0;
    std::optional<VendorSelection> vendorSelection = std::nullopt;
};

struct TaxRates {
    int printPercent = 0;
    int electronicPercent = 0;
};

struct AcquisitionConfig {
    TaxRates taxes;
    VendorSelection vendorSelection = VendorSelection::Cheapest;
};

enum class RecordType {
    Purchase,
    Cancellation
};

struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money preTaxCost;
    Money cost;  // final post-tax cost
    bool approved;
    std::string reason;

    RecordType type = RecordType::Purchase;

    // For a cancellation record, this is the original purchase order number.
    // It is 0 for normal purchase records.
    int relatedOrderNo = 0;
    std::string department;
    std::string vendor;
    AcquisitionStatus status = AcquisitionStatus::Requested;
    std::optional<VendorOffer> vendorOffer = std::nullopt;
    Timestamp timestamp = {};
};
struct AcquisitionSummary {
    std::size_t approved = 0, rejected = 0, cancelled = 0;
    Money totalPreTax, totalPostTax;
};
struct DepartmentSummary {
    std::string department;
    Money total, spent, remaining;
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget,
                    TaxRates taxRates = {});

    void setConfig(AcquisitionConfig config);
    AcquisitionConfig config() const { return {taxRates_, vendorMode_}; }
    Money quote(const std::string& id, int quantity, VendorSelection mode) const;
    const PurchaseRecord& purchase(const std::string& id, int quantity,
                                  VendorSelection mode, const std::string& department = {});
    PurchaseRecord submit(PurchaseRequest& request);
    AcquisitionStatus orderStatus(int orderNo) const;
    AcquisitionSummary summary() const;
    std::vector<DepartmentSummary> departmentSummaries() const;
    void setTaxRates(TaxRates taxRates);
    TaxRates taxRates() const { return taxRates_; }

    // Registers a separate budget for a department.
    void addDepartment(const std::string& name, Budget& budget);

    bool hasDepartment(const std::string& name) const;

    // Price of a request without buying anything. Throws NotFoundError.
    Money quote(const std::string& id, int quantity) const;

    // True if the purchase would be approved; if not, `reason` explains why.
    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr) const;

    bool canPurchase(const std::string& id, int quantity,
                    const std::string& department,
                    std::string* reason = nullptr) const;

    // Buys immediately. Throws NotFoundError, QuotaExceededError,
    // BudgetExceededError or std::invalid_argument. On success the budget
    // and holdings are updated and the record is added to history.
    const PurchaseRecord& purchase(const std::string& id, int quantity);

    const PurchaseRecord& purchase(const std::string& id, int quantity,
                                    const std::string& department);

    // Cancels a previously approved purchase order.
    // The original purchase remains in history and a separate cancellation
    // record is appended.
    const PurchaseRecord& cancel(int orderNo);

    // Processes requests in order; each is approved or rejected on its own
    // (never throws for a rejected request). Every outcome is recorded.
    // Allocation failures may still throw; prior non-atomic successes remain committed.
    std::vector<PurchaseRecord> processBatch(const std::vector<PurchaseRequest>& reqs);

    // If allOrNothing is true, the entire batch is rejected if any request
    // would fail. In that case budget usage and holdings are unchanged.
    std::vector<PurchaseRecord> processBatch(
                    const std::vector<PurchaseRequest>& reqs,
                    bool allOrNothing);

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalPreTax() const;
    Money totalSpent() const;

    void printReport(std::ostream& os) const;

private:
    friend struct PersistenceAccess;
    struct PriceChoice {
        Money preTaxCost;
        std::optional<VendorOffer> offer;
    };
    PriceChoice priceForPurchase(const Resource& resource, int quantity, VendorSelection mode) const;
    Money applyTax(const Resource& resource, Money preTaxCost) const;
    PurchaseRecord prepare(const PurchaseRequest& request) const;
    // Stage all allocating/calculating work, then swap into live state.
    void commitRecords(std::vector<PurchaseRecord>& records);
    Budget& budgetForDepartment(const std::string& department);
    const Budget& budgetForDepartment(const std::string& department) const;
    Catalog& catalog_;
    Budget& budget_;
    std::map<std::string, Budget*> departmentBudgets_;
    TaxRates taxRates_;
    VendorSelection vendorMode_ = VendorSelection::Cheapest;
    std::vector<PurchaseRecord> history_;
    int nextOrderNo_ = 1;
};

}  // namespace bookmgmt
