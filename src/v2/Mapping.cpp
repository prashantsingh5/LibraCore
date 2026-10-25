#include "libracore/Mapping.h"
#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>
#include <set>
#include <limits>
using namespace bookmgmt;
namespace libracore {
void validateNumericInputs(const Json &value) {
  static const std::set<std::string> minorKeys = {
    "unit_price_minor", "platform_fee_minor", "price_minor", "total_minor",
    "max_spend", "new_allocation", "minor_units"};
  static const std::set<std::string> integerKeys = {
    "quantity", "category", "publication_year", "edition", "binding", "issues_per_year",
    "subscription_years", "license", "format", "duration_minutes", "estimated_delivery_days",
    "fiscal_year", "max_units", "max_titles", "carry_percent", "order_id", "limit", "offset",
    "from_year", "to_year", "print_tax", "electronic_tax", "loan_days"};
  if (value.is_array()) {
    for (const auto &item : value) validateNumericInputs(item);
  } else if (value.is_object()) {
    for (auto it = value.begin(); it != value.end(); ++it) {
      const bool minor = minorKeys.count(it.key()) != 0;
      if ((minor || integerKeys.count(it.key())) && !it.value().is_null()) {
        const auto &n = it.value();
        if (!n.is_number_integer()) throw std::invalid_argument("Integer field required: " + it.key());
        const auto high = minor ? std::numeric_limits<std::int64_t>::max() : std::numeric_limits<int>::max();
        if (n.is_number_unsigned()) {
          if (n.get<std::uint64_t>() > static_cast<std::uint64_t>(high)) throw std::overflow_error("Integer field overflow");
        } else if (n.get<std::int64_t>() > high || (!minor && n.get<std::int64_t>() < std::numeric_limits<int>::min())) {
          throw std::overflow_error("Integer field overflow");
        }
      }
      validateNumericInputs(it.value());
    }
  }
}

Json money(std::int64_t n, const std::string &c) {
  auto m = Money::fromMinor(n, c);
  return {{"minor_units", m.minorUnits()},
          {"currency", m.currency()},
          {"formatted", m.toCurrencyString()}};
}
static Money price(const Json &j, const char *key = "unit_price_minor") {
  return Money::fromMinor(j.at(key).get<std::int64_t>(),
                          j.at("currency").get<std::string>());
}
static int bounded(const Json &j, const char *key, int def, int lo, int hi) {
  int v = j.value(key, def);
  if (v < lo || v > hi)
    throw std::invalid_argument(std::string("invalid ") + key);
  return v;
}
std::unique_ptr<Resource> resource(const Json &j) {
  validateNumericInputs(j);
  auto id = j.at("resource_id").get<std::string>(),
       title = j.at("title").get<std::string>(),
       pub = j.value("publisher", std::string{});
  int year = j.value("publication_year", 2026);
  auto p = price(j);
  auto type = j.at("resource_type").get<std::string>();
  auto authors = j.value("authors", std::vector<std::string>{});
  auto isbn = j.value("isbn", std::string{});
  auto url = j.value("access_url", std::string{});
  auto license = static_cast<LicenseModel>(bounded(j, "license", 1, 0, 1));
  auto fee = Money::fromMinor(j.value("platform_fee_minor", std::int64_t(0)),
                              j.value("platform_currency", p.currency()));
  std::unique_ptr<Resource> r;
  if (type == "Book")
    r = std::make_unique<Book>(
        id, title, authors, isbn, pub, year, p, j.value("edition", 1),
        static_cast<Binding>(bounded(j, "binding", 0, 0, 1)));
  else if (type == "Journal")
    r = std::make_unique<Journal>(id, title, j.value("issn", std::string{}),
                                  j.value("issues_per_year", 1), pub, year, p,
                                  j.value("subscription_years", 1));
  else if (type == "ElectronicResource")
    r = std::make_unique<ElectronicResource>(id, title, pub, year, p, url,
                                             license, fee);
  else if (type == "EBook")
    r = std::make_unique<EBook>(
        id, title, authors, isbn,
        static_cast<EBookFormat>(bounded(j, "format", 0, 0, 2)),
        j.value("drm", false), pub, year, p, url, license, fee);
  else if (type == "AudioBook")
    r = std::make_unique<AudioBook>(
        id, title, j.value("narrator", std::string{}),
        j.value("duration_minutes", 1), pub, year, p, url, license, fee);
  else if (type == "Thesis")
    r = std::make_unique<Thesis>(
        id, title, j.value("university", std::string{}),
        j.value("degree", std::string{}), j.value("supervisor", std::string{}),
        pub, year, p);
  else
    throw std::invalid_argument("unknown resource type");
  for (const auto &v : j.value("vendors", Json::array()))
    r->addVendor(
        {v.at("name"), Money::fromMinor(v.at("price_minor"), v.at("currency")),
         v.at("vendor_id"), v.at("rating"), v.at("estimated_delivery_days")});
  return r;
}
Json validateResource(const Json &j) {
  auto r = resource(j);
  Json out = j;
  out["currency"] = r->unitPrice().currency();
  out["category"] = static_cast<int>(r->category());
  if (auto *digital = dynamic_cast<ElectronicResource *>(r.get()))
    out["platform_currency"] = digital->platformFee().currency();
  out["price"] = money(r->unitPrice().minorUnits(), r->unitPrice().currency());
  return out;
}
std::string timestamp(Timestamp t) {
  auto raw = std::chrono::system_clock::to_time_t(t);
  std::tm tm{};
  gmtime_r(&raw, &tm);
  std::ostringstream s;
  s << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return s.str();
}
Timestamp timestamp(const std::string &s) {
  if (!std::regex_match(
          s,
          std::regex(
              R"(^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]+)?(Z|\+00:00|\+00)$)")))
    throw std::invalid_argument("UTC timestamp required");
  std::tm tm{};
  std::istringstream in(s.substr(0, 19));
  in >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
  if (in.fail())
    throw std::invalid_argument("invalid timestamp");
  // PostgreSQL UTC output can include fractional seconds and +00:00.
  auto suffix = s.substr(19);
  if (suffix.back() != 'Z' && suffix.find("+00:00") == std::string::npos &&
      suffix != "+00")
    throw std::invalid_argument("UTC timestamp required");
  if (tm.tm_year < 70 || tm.tm_year > 300)
    throw std::invalid_argument("Timestamp year must be 1970..2200");
  auto result = std::chrono::system_clock::from_time_t(timegm(&tm));
  if (timestamp(result).substr(0, 19) != s.substr(0, 19))
    throw std::invalid_argument("Invalid calendar timestamp");
  return result;
}
Json loan(const Loan &l) {
  return {{"loan_id", l.loanId},
          {"resource_id", l.resourceId},
          {"borrower_id", l.borrowerId},
          {"quantity", l.quantity},
          {"returned_quantity", l.returnedQuantity},
          {"borrowed_at", timestamp(l.borrowDate)},
          {"due_at", timestamp(l.dueDate)},
          {"returned_at",
           l.returnDate ? Json(timestamp(*l.returnDate)) : Json(nullptr)},
          {"status", static_cast<int>(l.status)}};
}
Loan loan(const Json &j) {
  Loan l;
  l.loanId = j.at("loan_id");
  l.resourceId = j.at("resource_id");
  l.borrowerId = j.at("borrower_id");
  l.quantity = j.at("quantity");
  l.returnedQuantity = j.at("returned_quantity");
  l.borrowDate = timestamp(j.at("borrowed_at"));
  l.dueDate = timestamp(j.at("due_at"));
  if (!j.at("returned_at").is_null())
    l.returnDate = timestamp(j.at("returned_at"));
  l.status = static_cast<LoanStatus>(j.at("status").get<int>());
  return l;
}
Json record(const PurchaseRecord &r) {
  Json v = nullptr;
  if (r.vendorOffer) {
    const auto &x = *r.vendorOffer;
    v = {{"name", x.vendor},
         {"vendor_id", x.vendorId},
         {"price_minor", x.price.minorUnits()},
         {"currency", x.price.currency()},
         {"rating", x.rating},
         {"estimated_delivery_days", x.deliveryDays}};
  }
  return {{"order_id", r.orderNo},
          {"requested_resource_id", r.resourceId},
          {"title", r.title},
          {"category", static_cast<int>(r.category)},
          {"quantity", r.quantity},
          {"pre_tax_minor", r.preTaxCost.minorUnits()},
          {"post_tax_minor", r.cost.minorUnits()},
          {"currency", r.cost.currency()},
          {"approved", r.approved},
          {"reason", r.reason},
          {"record_type", static_cast<int>(r.type)},
          {"related_order_id",
           r.relatedOrderNo ? Json(r.relatedOrderNo) : Json(nullptr)},
          {"requested_department", r.department},
          {"vendor", r.vendor},
          {"status", static_cast<int>(r.status)},
          {"vendor_snapshot", v},
          {"created_at", timestamp(r.timestamp)},
          {"cost", money(r.cost.minorUnits(), r.cost.currency())}};
}
PurchaseRecord record(const Json &j) {
  PurchaseRecord r{};
  r.orderNo = j.at("order_id");
  r.resourceId = j.at("requested_resource_id");
  r.title = j.at("title");
  r.category = static_cast<ResourceCategory>(j.at("category").get<int>());
  r.quantity = j.at("quantity");
  r.preTaxCost = price(j, "pre_tax_minor");
  r.cost = price(j, "post_tax_minor");
  r.approved = j.at("approved");
  r.reason = j.at("reason");
  r.type = static_cast<RecordType>(j.at("record_type").get<int>());
  r.relatedOrderNo = j.at("related_order_id").is_null()
                         ? 0
                         : j.at("related_order_id").get<int>();
  r.department = j.at("requested_department");
  r.vendor = j.at("vendor");
  r.status = static_cast<AcquisitionStatus>(j.at("status").get<int>());
  r.timestamp = timestamp(j.at("created_at"));
  if (!j.at("vendor_snapshot").is_null()) {
    auto v = j.at("vendor_snapshot");
    r.vendorOffer = VendorOffer{
        v.at("name"), Money::fromMinor(v.at("price_minor"), v.at("currency")),
        v.at("vendor_id"), v.at("rating"), v.at("estimated_delivery_days")};
  }
  return r;
}
Json budgetSummary(const Budget &b) {
  auto s = b.summary();
  Json out = {
      {"total", money(s.total.minorUnits(), s.total.currency())},
      {"spent", money(s.spent.minorUnits(), s.spent.currency())},
      {"remaining", money(s.remaining.minorUnits(), s.remaining.currency())},
      {"percentage_used", s.percentageUsed},
      {"categories", Json::array()}};
  for (int i = 0; i < 4; ++i) {
    auto c = b.categorySummary(static_cast<ResourceCategory>(i));
    auto opt = [](auto x) -> Json { return x ? Json(*x) : Json(nullptr); };
    out["categories"].push_back(
        {{"category", i},
         {"units_used", c.unitsUsed},
         {"units_remaining", opt(c.unitsRemaining)},
         {"titles_used", c.titlesUsed},
         {"titles_remaining", opt(c.titlesRemaining)},
         {"spend", money(c.spend.minorUnits(), c.spend.currency())},
         {"spend_remaining", c.spendRemaining
                                 ? money(c.spendRemaining->minorUnits(),
                                         c.spendRemaining->currency())
                                 : Json(nullptr)},
         {"unit_utilization", opt(c.unitUtilization)},
         {"title_utilization", opt(c.titleUtilization)},
         {"spend_utilization", opt(c.spendUtilization)}});
  }
  return out;
}
} // namespace libracore
