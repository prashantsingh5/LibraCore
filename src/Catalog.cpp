#include "bookmgmt/Catalog.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <limits>
#include "bookmgmt/Validation.h"

#include "bookmgmt/Book.h"
#include "bookmgmt/MT26076_EBook.h"
#include "bookmgmt/Exceptions.h"
#include "bookmgmt/MT26076_Journal.h"

namespace bookmgmt {

namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}  // namespace

Resource& Catalog::add(std::unique_ptr<Resource> r) {
    if (!r) throw std::invalid_argument("cannot add a null resource");
    const std::string id = r->id();
    if (items_.count(id)) throw DuplicateIdError(id);
    Entry& e = items_[id];
    e.resource = std::move(r);
    return *e.resource;
}

bool Catalog::contains(const std::string& id) const { return items_.count(id) > 0; }

Resource* Catalog::find(const std::string& id) {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

const Resource* Catalog::find(const std::string& id) const {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

Resource& Catalog::get(const std::string& id) {
    requireId(id, "resource id");
    if (Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

const Resource& Catalog::get(const std::string& id) const {
    requireId(id, "resource id");
    if (const Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

void Catalog::remove(const std::string& id) {
    requireId(id, "resource id");
    if (lent(id) != 0) throw std::invalid_argument("cannot remove a resource with active loans");
    if (items_.erase(id) == 0) throw NotFoundError(id);
}

int Catalog::holdings(const std::string& id) const {
    requireId(id, "resource id");
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return it->second.holdings;
}

void Catalog::addHoldings(const std::string& id, int units) {
    requireId(id, "resource id");
    auto it = items_.find(id);

    if (it == items_.end()) {
        throw NotFoundError(id);
    }

    const auto newHoldings =
        static_cast<long long>(it->second.holdings) + units;
    if (newHoldings > std::numeric_limits<int>::max())
        throw std::overflow_error("holdings overflow");

    if (newHoldings < 0) {
        throw std::invalid_argument(
            "holdings cannot become negative");
    }

    // Q14: Existing loans/sessions must remain backed by holdings.
    if (newHoldings < it->second.lent) {
        throw std::invalid_argument(
            "cannot reduce holdings below currently lent units");
    }

    it->second.holdings = static_cast<int>(newHoldings);
}

int Catalog::lent(const std::string& id) const {
    requireId(id, "resource id");
    auto it = items_.find(id);

    if (it == items_.end()) {
        throw NotFoundError(id);
    }

    return it->second.lent;
}

int Catalog::available(const std::string& id) const {
    requireId(id, "resource id");
    auto it = items_.find(id);

    if (it == items_.end()) {
        throw NotFoundError(id);
    }

    return it->second.holdings -
           it->second.lent;
}

std::vector<const Resource*> Catalog::where(
    const std::function<bool(const Resource&)>& pred) const {
    std::vector<const Resource*> out;
    for (const auto& [id, entry] : items_)
        if (pred(*entry.resource)) out.push_back(entry.resource.get());
    return out;
}

std::vector<const Resource*> Catalog::all() const {
    return where([](const Resource&) { return true; });
}

std::vector<const Resource*> Catalog::byCategory(ResourceCategory c) const {
    return where([c](const Resource& r) { return r.category() == c; });
}

namespace {
bool authorMatches(const Resource& r, const std::string& needle) {
    const std::vector<std::string>* authors = nullptr;
    if (const auto* b = dynamic_cast<const Book*>(&r)) authors = &b->authors();
    if (const auto* b = dynamic_cast<const EBook*>(&r)) authors = &b->authors();
    if (!authors) return false;
    return std::any_of(authors->begin(), authors->end(), [&](const std::string& a) {
        return lower(a).find(needle) != std::string::npos;
    });
}
bool identifierMatches(const Resource& r, const std::string& needle) {
    if (const auto* b = dynamic_cast<const Book*>(&r)) return lower(b->isbn()) == needle;
    if (const auto* b = dynamic_cast<const EBook*>(&r)) return lower(b->isbn()) == needle;
    if (const auto* j = dynamic_cast<const Journal*>(&r)) return lower(j->issn()) == needle;
    return false;
}
}
std::vector<const Resource*> Catalog::search(const SearchQuery& q) const {
    if (q.fromYear && q.toYear && *q.fromYear > *q.toYear)
        throw std::invalid_argument("from year must not be greater than to year");
    const auto title = q.title ? lower(*q.title) : std::string{};
    const auto author = q.author ? lower(*q.author) : std::string{};
    const auto identifier = q.identifier ? lower(*q.identifier) : std::string{};
    auto out = where([&](const Resource& r) {
        return (!q.title || lower(r.title()).find(title) != std::string::npos)
            && (!q.author || authorMatches(r, author))
            && (!q.identifier || identifierMatches(r, identifier))
            && (!q.category || r.category() == *q.category)
            && (!q.fromYear || r.year() >= *q.fromYear)
            && (!q.toYear || r.year() <= *q.toYear);
    });
    if (q.sort == SearchSort::PriceAscending || q.sort == SearchSort::PriceDescending) {
        for (const auto* r : out)
            if (r->unitPrice().currency() != out.front()->unitPrice().currency())
                throw std::invalid_argument("cannot sort prices in mixed currencies");
    }
    if (q.sort == SearchSort::Id) return out; // map traversal already provides ID order
    std::sort(out.begin(), out.end(), [&](const Resource* a, const Resource* b) {
        switch (q.sort) {
            case SearchSort::TitleAscending:
            case SearchSort::TitleDescending: {
                auto x = lower(a->title()), y = lower(b->title());
                if (x != y) return q.sort == SearchSort::TitleAscending ? x < y : x > y;
                break;
            }
            case SearchSort::YearAscending:
            case SearchSort::YearDescending:
                if (a->year() != b->year()) return q.sort == SearchSort::YearAscending
                    ? a->year() < b->year() : a->year() > b->year();
                break;
            case SearchSort::PriceAscending:
            case SearchSort::PriceDescending:
                if (a->unitPrice() != b->unitPrice()) return q.sort == SearchSort::PriceAscending
                    ? a->unitPrice() < b->unitPrice() : a->unitPrice() > b->unitPrice();
                break;
            case SearchSort::Id: break;
        }
        return a->id() < b->id();
    });
    return out;
}
std::vector<const Resource*> Catalog::searchTitle(const std::string& text) const {
    SearchQuery q; q.title = text; return search(q);
}
std::vector<const Resource*> Catalog::searchAuthor(const std::string& text) const {
    SearchQuery q; q.author = text; return search(q);
}
std::vector<const Resource*> Catalog::searchIdentifier(const std::string& text) const {
    SearchQuery q; q.identifier = text; return search(q);
}
std::vector<const Resource*> Catalog::searchISBN(const std::string& text) const {
    auto out = searchIdentifier(text);
    out.erase(std::remove_if(out.begin(), out.end(), [](const Resource* r) {
        return dynamic_cast<const Journal*>(r) != nullptr;
    }), out.end());
    return out;
}
std::vector<const Resource*> Catalog::searchISSN(const std::string& text) const {
    SearchQuery q; q.identifier = text; q.category = ResourceCategory::Journal; return search(q);
}
std::vector<const Resource*> Catalog::searchYearRange(int fromYear, int toYear) const {
    SearchQuery q; q.fromYear = fromYear; q.toYear = toYear; return search(q);
}
} // namespace bookmgmt
