#include "bookmgmt/Budget.h"
#include "bookmgmt/Validation.h"
#include <limits>

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {
// Every category, in the order Budget::print() lists them.
const ResourceCategory kAllCategories[] = {ResourceCategory::Book,
                                           ResourceCategory::ElectronicResource,
                                           ResourceCategory::Journal,
                                           ResourceCategory::EBook};
}

Budget::Budget(Money total)
    : total_(total),
      spent_(Money::fromMinor(0, total.currency())) {

    if (total_.isNegative()) {
        throw std::invalid_argument(
            "budget must not be negative");
    }
}

void Budget::swap(Budget& other) noexcept {
    using std::swap;
    swap(total_, other.total_); swap(spent_, other.spent_);
    quotas_.swap(other.quotas_); usage_.swap(other.usage_);
    purchasedTitles_.swap(other.purchasedTitles_);
}

Budget Budget::nextYear(Money newAllocation,
                        int carryPercent) const {
    if (newAllocation.isNegative()) {
        throw std::invalid_argument(
            "new budget allocation must not be negative");
    }

    if (carryPercent < 0 || carryPercent > 100) {
        throw std::invalid_argument(
            "carry percentage must be between 0 and 100");
    }

    const Money carryAmount =
        remaining().scaledRatio(carryPercent, 100);

    Budget next(newAllocation + carryAmount);

    // Preserve quota policy for the new year.
    next.quotas_ = quotas_;

    return next;
}

void Budget::setQuota(ResourceCategory c, Quota q) {
    if (q.maxUnits < 0 || q.maxSpend.isNegative() ||
        (q.maxTitles && *q.maxTitles < 0)) {
        throw std::invalid_argument("quota limits must not be negative");
    }

    if (q.maxSpend.currency() != total_.currency()) {
        throw std::invalid_argument(
            "quota currency must match budget currency");
    }

    const auto current = usageFor(c);
    if (q.maxUnits < current.units || q.maxSpend < current.spent ||
        (q.maxTitles && *q.maxTitles < titlesUsed(c)))
        throw std::invalid_argument("quota cannot be lowered below current usage");
    quotas_[c] = q;
}

void Budget::removeQuota(ResourceCategory c) { quotas_.erase(c); }

std::optional<Quota> Budget::quotaFor(ResourceCategory c) const {
    auto it = quotas_.find(c);
    if (it == quotas_.end()) return std::nullopt;
    return it->second;
}

Usage Budget::usageFor(ResourceCategory c) const {
    auto it = usage_.find(c);

    if (it == usage_.end()) {
        return Usage{
            0,
            Money::fromMinor(
                0,
                total_.currency())
        };
    }

    return it->second;
}

std::optional<int> Budget::unitsRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxUnits - usageFor(c).units;
}

std::optional<Money> Budget::spendRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxSpend - usageFor(c).spent;
}

int Budget::titlesUsed(ResourceCategory c) const {
    auto it = purchasedTitles_.find(c);

    if (it == purchasedTitles_.end()) {
        return 0;
    }

    return static_cast<int>(it->second.size());
}

std::optional<int> Budget::titlesRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);

    if (!q || !q->maxTitles) {
        return std::nullopt;
    }

    return *q->maxTitles - titlesUsed(c);
}

Budget::Failure Budget::evaluate(ResourceCategory c, int units, Money cost,
                                 const std::string* resourceId,
                                 std::string& why) const {
    if (resourceId && !validId(*resourceId)) {
        why = "resource id must not be blank";
        return Failure::BadInput;
    }
    if (units > 0 && usageFor(c).units > std::numeric_limits<int>::max() - units) {
        why = "category units overflow";
        return Failure::BadInput;
    }
    if (units <= 0) {
        why = "quantity must be positive";
        return Failure::BadInput;
    }

    if (cost.isNegative()) {
        why = "cost must not be negative";
        return Failure::BadInput;
    }

    if (cost.currency() != total_.currency()) {
        why = "cost currency must match budget currency";
        return Failure::BadInput;
    }
    
    if (auto left = unitsRemaining(c); left && units > *left) {
        why = std::string(categoryName(c)) +
              " unit quota exceeded: requested " +
              std::to_string(units) + ", " +
              std::to_string(*left) + " remaining";
        return Failure::Quota;
    }

    if (auto left = spendRemaining(c); left && cost > *left) {
        why = std::string(categoryName(c)) +
              " spend quota exceeded: cost " +
              cost.toString() + ", " +
              left->toString() + " remaining";
        return Failure::Quota;
    }

    // Q7: only a resource that has not already been bought in this
    // category consumes another title slot.
    if (resourceId) {
        auto q = quotaFor(c);

        if (q && q->maxTitles) {
            const auto titlesIt = purchasedTitles_.find(c);

            const bool alreadyPurchased =
                titlesIt != purchasedTitles_.end() &&
                titlesIt->second.find(*resourceId) != titlesIt->second.end();

            if (!alreadyPurchased &&
                titlesUsed(c) >= *q->maxTitles) {
                why = std::string(categoryName(c)) +
                      " title quota exceeded: maximum " +
                      std::to_string(*q->maxTitles) +
                      " different titles";
                return Failure::Quota;
            }
        }
    }

    if (cost > remaining()) {
        why = "overall budget exceeded: cost " +
              cost.toString() + ", " +
              remaining().toString() + " remaining";
        return Failure::Overall;
    }

    why.clear();
    return Failure::None;
}

std::string Budget::check(ResourceCategory c, int units, Money cost) const {
    std::string why;
    evaluate(c, units, cost, nullptr, why);
    return why;
}

std::string Budget::check(ResourceCategory c,
                          const std::string& resourceId,
                          int units, Money cost) const {
    std::string why;
    evaluate(c, units, cost, &resourceId, why);
    return why;
}

void Budget::commit(ResourceCategory c, int units, Money cost) {
    Budget staged = *this;
    staged.commitUnchecked(c, units, cost);
    swap(staged);
}
void Budget::commit(ResourceCategory c, const std::string& id, int units, Money cost) {
    Budget staged = *this;
    staged.commitUnchecked(c, id, units, cost);
    swap(staged);
}

void Budget::commitUnchecked(
    ResourceCategory c,
    int units,
    Money cost) {

    std::string why;

    switch (evaluate(
        c,
        units,
        cost,
        nullptr,
        why)) {

        case Failure::None:
            break;

        case Failure::BadInput:
            throw std::invalid_argument(why);

        case Failure::Quota:
            throw QuotaExceededError(why);

        case Failure::Overall:
            throw BudgetExceededError(why);
    }

    auto [it, inserted] =
        usage_.try_emplace(
            c,
            Usage{
                0,
                Money::fromMinor(
                    0,
                    total_.currency())
            });

    Usage& u = it->second;

    u.units += units;
    u.spent += cost;
    spent_ += cost;
}

void Budget::commitUnchecked(
    ResourceCategory c,
    const std::string& resourceId,
    int units,
    Money cost) {

    std::string why;

    switch (evaluate(
        c,
        units,
        cost,
        &resourceId,
        why)) {

        case Failure::None:
            break;

        case Failure::BadInput:
            throw std::invalid_argument(why);

        case Failure::Quota:
            throw QuotaExceededError(why);

        case Failure::Overall:
            throw BudgetExceededError(why);
    }

    auto [it, inserted] =
        usage_.try_emplace(
            c,
            Usage{
                0,
                Money::fromMinor(
                    0,
                    total_.currency())
            });

    Usage& u = it->second;

    u.units += units;
    u.spent += cost;
    spent_ += cost;

    purchasedTitles_[c][resourceId] += units;
}

void Budget::refund(ResourceCategory c,
                    const std::string& resourceId,
                    int units,
                    Money cost) {
    requireId(resourceId, "resource id");
    if (units <= 0) {
        throw std::invalid_argument("refund quantity must be positive");
    }

    if (cost.isNegative()) {
        throw std::invalid_argument("refund cost must not be negative");
    }

    if (cost.currency() != total_.currency()) {
        throw std::invalid_argument(
            "refund currency must match budget currency");
    }

    auto usageIt = usage_.find(c);

    if (usageIt == usage_.end() ||
        units > usageIt->second.units ||
        cost > usageIt->second.spent ||
        cost > spent_) {
        throw std::invalid_argument("refund exceeds recorded budget usage");
    }

    auto categoryIt = purchasedTitles_.find(c);

    if (categoryIt == purchasedTitles_.end()) {
        throw std::invalid_argument("resource has no recorded purchase");
    }

    auto titleIt = categoryIt->second.find(resourceId);

    if (titleIt == categoryIt->second.end() ||
        units > titleIt->second) {
        throw std::invalid_argument("refund exceeds recorded resource usage");
    }

    // All validation is complete before any state is changed.
    usageIt->second.units -= units;
    usageIt->second.spent -= cost;
    spent_ -= cost;

    titleIt->second -= units;

    if (titleIt->second == 0) {
        categoryIt->second.erase(titleIt);
    }

    if (categoryIt->second.empty()) {
        purchasedTitles_.erase(categoryIt);
    }
}

namespace {
double percentage(double used, double limit) { return limit == 0 ? 0.0 : used * 100.0 / limit; }
}
BudgetSummary Budget::summary() const {
    return {total_, spent_, remaining(), percentage(spent_.toDouble(), total_.toDouble())};
}
CategorySummary Budget::categorySummary(ResourceCategory c) const {
    const auto u = usageFor(c); const auto q = quotaFor(c);
    CategorySummary s{c, u.units, unitsRemaining(c), titlesUsed(c), titlesRemaining(c),
                      u.spent, spendRemaining(c), std::nullopt, std::nullopt, std::nullopt};
    if (q) {
        s.unitUtilization = percentage(u.units, q->maxUnits);
        s.spendUtilization = percentage(u.spent.toDouble(), q->maxSpend.toDouble());
        if (q->maxTitles) s.titleUtilization = percentage(s.titlesUsed, *q->maxTitles);
    }
    return s;
}

void Budget::print(std::ostream& os) const {
    os << "Budget: total " << total_ << ", spent " << spent_ << ", remaining "
       << remaining() << "\n";
    os << std::left
        << std::setw(22) << "  Category"
        << std::setw(18) << "Units used/max"
        << std::setw(18) << "Titles used/max"
        << "Spend used/max\n";
    for (ResourceCategory c : kAllCategories) {
        const Usage u = usageFor(c);
        const auto q = quotaFor(c);
        const std::string units =
            std::to_string(u.units) + "/" + (q ? std::to_string(q->maxUnits) : "-");
        const std::string titles =
            std::to_string(titlesUsed(c)) + "/" +
            (q && q->maxTitles ? std::to_string(*q->maxTitles) : "-");
            const std::string spend =
            u.spent.toString() + "/" + (q ? q->maxSpend.toString() : "-");
        os << "  " << std::setw(20) << categoryName(c)
            << std::setw(18) << units
            << std::setw(18) << titles
            << spend << "\n";
                }
}

}  // namespace bookmgmt
