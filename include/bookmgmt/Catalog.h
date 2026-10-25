#pragma once
// Catalog: owns every Resource, keyed by its id, and tracks copies/seats held.

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "bookmgmt/Resource.h"
#include "bookmgmt/History.h"

namespace bookmgmt {

struct PersistenceAccess; // V2 adapter: trusted relational state restoration only.

enum class SearchSort { Id, TitleAscending, TitleDescending, YearAscending,
                        YearDescending, PriceAscending, PriceDescending };
struct SearchQuery {
    std::optional<std::string> title, author, identifier;
    std::optional<ResourceCategory> category;
    std::optional<int> fromYear, toYear;
    SearchSort sort = SearchSort::Id;
};

class Catalog {
public:
    explicit Catalog(LendingConfig config = {});
    void setLendingConfig(LendingConfig config);
    LendingConfig lendingConfig() const { return lendingConfig_; }
    Loan borrow(const std::string& resourceId, const std::string& borrowerId,
                int quantity = 1, Timestamp when = std::chrono::system_clock::now(),
                std::optional<Timestamp> due = std::nullopt);
    // Named loans are returned by loan ID, never by a resource-wide legacy return.
    void returnBorrowed(const std::string& loanId, int quantity,
                        Timestamp when = std::chrono::system_clock::now());
    void returnLoan(const std::string& loanId, Timestamp when);
    Loan loan(const std::string& loanId) const;
    std::vector<Loan> activeLoans() const;
    std::vector<Loan> loansForUser(const std::string& borrowerId) const;
    std::vector<Loan> loansForResource(const std::string& resourceId) const;
    LendingSummary lendingSummary() const;
    std::vector<Event> events() const;
    std::vector<const Resource*> search(const SearchQuery& query = {}) const;

    // Takes ownership. Throws DuplicateIdError if the id is already present.
    Resource& add(std::unique_ptr<Resource> r);

    // Convenience: catalog.emplace<Book>(ctor args...)
    template <typename T, typename... Args>
    T& emplace(Args&&... args) {
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *p;
        add(std::move(p));
        return ref;
    }

    bool contains(const std::string& id) const;
    Resource* find(const std::string& id);              // nullptr if absent
    const Resource* find(const std::string& id) const;  // nullptr if absent
    Resource& get(const std::string& id);               // throws NotFoundError
    const Resource& get(const std::string& id) const;   // throws NotFoundError
    void remove(const std::string& id);                 // throws NotFoundError

    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }

    // Q14: Lending / concurrent-use tracking.
    //
    // For print resources, "lent" means copies currently on loan.
    // For digital resources, "lent" means concurrent active sessions.
    int lent(const std::string& id) const;
    int available(const std::string& id) const;

    // Lends copies or starts concurrent electronic sessions.
    // Throws NotFoundError for an unknown resource and
    // std::invalid_argument for invalid quantities or insufficient availability.
    void lend(const std::string& id, int units = 1);

    // Returns copies or ends concurrent electronic sessions.
    void returnLoan(const std::string& id, int units = 1);

    // Holdings: number of copies (print) or seats (electronic) owned.
    int holdings(const std::string& id) const;
    void addHoldings(const std::string& id, int units);

    // Queries. Results are in id order and point into the catalog.
    std::vector<const Resource*> all() const;
    std::vector<const Resource*> byCategory(ResourceCategory c) const;

    std::vector<const Resource*> searchTitle(
        const std::string& text) const;  // case-insensitive

    // Q13: Search books and ebooks by author.
    // Matching is case-insensitive and allows partial author names.
    std::vector<const Resource*> searchAuthor(
        const std::string& author) const;

    // Q13: Search exact ISBN across Book and EBook resources.
    std::vector<const Resource*> searchISBN(
        const std::string& isbn) const;

    // Q13: Search exact ISSN across Journal resources.
    std::vector<const Resource*> searchISSN(
        const std::string& issn) const;

    // Q13: Convenience search across both ISBN and ISSN.
    std::vector<const Resource*> searchIdentifier(
        const std::string& identifier) const;

    // Q13: Inclusive publication-year range.
    std::vector<const Resource*> searchYearRange(
        int fromYear,
        int toYear) const;

    std::vector<const Resource*> where(
        const std::function<bool(const Resource&)>& pred) const;

private:
    friend struct PersistenceAccess;
    friend class AcquisitionManager;
    Loan borrowImpl(const std::string&, const std::string&, int, Timestamp,
                    std::optional<Timestamp>, bool);
    LendingConfig lendingConfig_;
    std::map<std::string, Loan> loans_;
    std::vector<Event> events_;
    std::uint64_t nextLoan_ = 1;
struct Entry {
    std::unique_ptr<Resource> resource;
    int holdings = 0;

    // Q14:
    // Print resource  -> copies currently lent.
    // Digital resource -> active concurrent sessions.
    int lent = 0;
};
    std::map<std::string, Entry> items_;
};

}  // namespace bookmgmt
