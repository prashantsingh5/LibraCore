#include "bookmgmt/Catalog.h"
#include "bookmgmt/Validation.h"
#include "bookmgmt/Exceptions.h"
#include <algorithm>
#include <limits>
namespace bookmgmt {
Catalog::Catalog(LendingConfig config) { setLendingConfig(config); }
void Catalog::setLendingConfig(LendingConfig config) {
    if (config.defaultLoanDays <= 0 || config.defaultLoanDays > 36500)
        throw std::invalid_argument("loan duration must be 1..36500 days");
    lendingConfig_ = config;
}
Loan Catalog::borrow(const std::string& id, const std::string& user, int qty,
                     Timestamp when, std::optional<Timestamp> due) {
    return borrowImpl(id, user, qty, when, due, false);
}
Loan Catalog::borrowImpl(const std::string& id, const std::string& user, int qty,
                         Timestamp when, std::optional<Timestamp> due, bool anonymous) {
    requireId(id, "resource id"); requireId(user, "borrower id");
    if (qty <= 0) throw std::invalid_argument("loan quantity must be positive");
    if (qty > available(id)) throw std::invalid_argument("insufficient availability");
    if (!due) {
        auto duration = std::chrono::hours(24LL * lendingConfig_.defaultLoanDays);
        if (when > Timestamp::max() - duration) throw std::invalid_argument("due date overflow");
        due = when + duration;
    }
    if (*due <= when) throw std::invalid_argument("due date must follow borrow date");
    if (nextLoan_ == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("loan id exhausted");
    Loan result{"L" + std::to_string(nextLoan_), id, user, qty, when, *due,
                std::nullopt, LoanStatus::Active, 0, anonymous};
    auto stagedLoans = loans_;
    auto stagedEvents = events_;
    stagedLoans.emplace(result.loanId, result);
    stagedEvents.push_back({EventType::Borrow, id, qty, user,
                           std::chrono::system_clock::now(), result.loanId});
    loans_.swap(stagedLoans); events_.swap(stagedEvents);
    items_.at(id).lent += qty;
    ++nextLoan_;
    return result;
}
Loan Catalog::loan(const std::string& id) const {
    requireId(id, "loan id");
    auto it = loans_.find(id);
    if (it == loans_.end()) throw std::invalid_argument("loan not found: " + id);
    return it->second;
}
void Catalog::returnBorrowed(const std::string& id, int qty, Timestamp when) {
    const Loan original = loan(id);
    if (qty <= 0 || qty > original.outstanding())
        throw std::invalid_argument("return quantity exceeds outstanding loan or is nonpositive");
    if (when < original.borrowDate) throw std::invalid_argument("return precedes borrow date");
    auto stagedLoans = loans_;
    auto stagedEvents = events_;
    auto& changed = stagedLoans.at(id);
    changed.returnedQuantity += qty;
    if (changed.outstanding() == 0) { changed.status = LoanStatus::Returned; changed.returnDate = when; }
    stagedEvents.push_back({EventType::Return, original.resourceId, qty, original.borrowerId,
                           std::chrono::system_clock::now(), id});
    loans_.swap(stagedLoans); events_.swap(stagedEvents);
    items_.at(original.resourceId).lent -= qty;
}
void Catalog::returnLoan(const std::string& id, Timestamp when) {
    returnBorrowed(id, loan(id).outstanding(), when);
}
void Catalog::lend(const std::string& id, int qty) {
    borrowImpl(id, "anonymous", qty, std::chrono::system_clock::now(), std::nullopt, true);
}
void Catalog::returnLoan(const std::string& id, int qty) {
    (void)get(id);
    if (qty <= 0) throw std::invalid_argument("return quantity must be positive");
    int anonymousUnits = 0;
    for (const auto& [key, l] : loans_)
        if (l.resourceId == id && l.anonymous) anonymousUnits += l.outstanding();
    if (qty > anonymousUnits) throw std::invalid_argument("return exceeds anonymous loans; use loan ID for named borrowers");
    auto stagedLoans = loans_;
    auto stagedEvents = events_;
    int left = qty;
    const auto now = std::chrono::system_clock::now();
    for (auto& [key, l] : stagedLoans) {
        if (!left) break;
        if (l.resourceId != id || !l.anonymous || !l.outstanding()) continue;
        const int n = std::min(left, l.outstanding());
        l.returnedQuantity += n; left -= n;
        if (!l.outstanding()) { l.status = LoanStatus::Returned; l.returnDate = now; }
        stagedEvents.push_back({EventType::Return, id, n, l.borrowerId, now, key});
    }
    loans_.swap(stagedLoans); events_.swap(stagedEvents);
    items_.at(id).lent -= qty;
}
std::vector<Loan> Catalog::activeLoans() const {
    std::vector<Loan> out;
    for (const auto& [id, l] : loans_) if (l.status == LoanStatus::Active) out.push_back(l);
    return out;
}
std::vector<Loan> Catalog::loansForUser(const std::string& user) const {
    requireId(user, "borrower id"); std::vector<Loan> out;
    for (const auto& [id, l] : loans_) if (l.borrowerId == user) out.push_back(l);
    return out;
}
std::vector<Loan> Catalog::loansForResource(const std::string& resource) const {
    requireId(resource, "resource id"); std::vector<Loan> out;
    for (const auto& [id, l] : loans_) if (l.resourceId == resource) out.push_back(l);
    return out;
}
LendingSummary Catalog::lendingSummary() const {
    LendingSummary s;
    for (const auto& [id, e] : items_) { s.totalHoldings += e.holdings; s.borrowed += e.lent; }
    s.available = s.totalHoldings - s.borrowed;
    for (const auto& [id, l] : loans_) if (l.status == LoanStatus::Active) ++s.activeLoans;
    return s;
}
std::vector<Event> Catalog::events() const {
    auto out = events_;
    std::stable_sort(out.begin(), out.end(), [](const Event& a, const Event& b) {
        return a.timestamp < b.timestamp;
    });
    return out;
}
}
