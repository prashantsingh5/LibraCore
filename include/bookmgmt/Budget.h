#pragma once
// Budget: an overall spending limit plus optional per-category purchase quotas.
//
// A quota caps how many units (copies/seats) and how much money may be spent on
// one category. Categories with no quota are limited only by the total budget.

#include <iosfwd>
#include <map>
#include <optional>
#include <string>

#include "bookmgmt/Money.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt {

struct PersistenceAccess; // V2 adapter: trusted relational state restoration only.

struct Quota {
    int maxUnits;                       // maximum copies/seats
    Money maxSpend;                     // maximum money that may be spent
    std::optional<int> maxTitles = {};  // maximum different titles; no value = unlimited
};

struct Usage {
    int units = 0;
    Money spent;
};

struct BudgetSummary {
    Money total, spent, remaining;
    double percentageUsed;
};
struct CategorySummary {
    ResourceCategory category;
    int unitsUsed;
    std::optional<int> unitsRemaining;
    int titlesUsed;
    std::optional<int> titlesRemaining;
    Money spend;
    std::optional<Money> spendRemaining;
    std::optional<double> unitUtilization, titleUtilization, spendUtilization;
};

class Budget {
public:
    explicit Budget(Money total);
    void swap(Budget& other) noexcept;
    BudgetSummary summary() const;
    CategorySummary categorySummary(ResourceCategory category) const;
    Budget nextYear(Money newAllocation, int carryPercent) const;

    Money total() const { return total_; }
    Money spent() const { return spent_; }
    Money remaining() const { return total_ - spent_; }

    void setQuota(ResourceCategory c, Quota q);
    void removeQuota(ResourceCategory c);
    std::optional<Quota> quotaFor(ResourceCategory c) const;
    Usage usageFor(ResourceCategory c) const;

    // Remaining allowance in a category; nullopt means "no quota set".
    std::optional<int> unitsRemaining(ResourceCategory c) const;
    std::optional<Money> spendRemaining(ResourceCategory c) const;
    int titlesUsed(ResourceCategory c) const;
    std::optional<int> titlesRemaining(ResourceCategory c) const;

    // Returns an empty string if the purchase fits, otherwise the reason it
    // does not. Does not change state.
    std::string check(ResourceCategory c, int units, Money cost) const;
    std::string check(ResourceCategory c, const std::string& resourceId,
                      int units, Money cost) const;

    // Records a purchase. Throws QuotaExceededError / BudgetExceededError
    // (and changes nothing) if it would not fit.
    void commit(ResourceCategory c, int units, Money cost);
    void commit(ResourceCategory c, const std::string& resourceId,
                int units, Money cost);

    // Reverses a previously committed purchase.
    void refund(ResourceCategory c, const std::string& resourceId,
                int units, Money cost);
    void print(std::ostream& os) const;

private:
    friend struct PersistenceAccess;
    void commitUnchecked(ResourceCategory, int, Money);
    void commitUnchecked(ResourceCategory, const std::string&, int, Money);
    enum class Failure { None, BadInput, Quota, Overall };
    Failure evaluate(ResourceCategory c, int units, Money cost,
                 const std::string* resourceId,
                 std::string& why) const;

    Money total_;
    Money spent_;
    std::map<ResourceCategory, Quota> quotas_;
    std::map<ResourceCategory, Usage> usage_;
    std::map<ResourceCategory, std::map<std::string, int>> purchasedTitles_;
};

}  // namespace bookmgmt
