#include "bookmgmt/Exceptions.h"
#include "bookmgmt/Validation.h"
#include "libracore/Mapping.h"
#include <limits>
#include <pqxx/pqxx>
#include <set>
#include <condition_variable>
#include <mutex>
#include <vector>
using namespace bookmgmt;
namespace libracore {
// Bounded RAII leases. Transaction is destroyed before connection returns to pool.
class ConnectionPool {
  std::mutex mutex_;
  std::condition_variable ready_;
  std::vector<std::unique_ptr<pqxx::connection>> idle_;
  unsigned count_=0;
  std::string url_;
public:
  explicit ConnectionPool(std::string url):url_(std::move(url)) {}
  std::unique_ptr<pqxx::connection> acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!ready_.wait_for(lock,std::chrono::seconds(5),[&]{return !idle_.empty() || count_<4;}))
      throw Error("DATABASE_ERROR","Database connection pool busy");
    if (!idle_.empty()) { auto c=std::move(idle_.back());idle_.pop_back();return c; }
    ++count_; lock.unlock();
    try {return std::make_unique<pqxx::connection>(url_);}
    catch (...) {lock.lock();--count_;ready_.notify_one();throw;}
  }
  void release(std::unique_ptr<pqxx::connection> c) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (c && c->is_open()) {try {idle_.push_back(std::move(c));} catch (...) {--count_;}}
    else --count_;
    ready_.notify_one();
  }
};
namespace {
struct Lease {
  std::shared_ptr<ConnectionPool> pool;
  std::unique_ptr<pqxx::connection> connection;
  explicit Lease(std::shared_ptr<ConnectionPool> p):pool(std::move(p)),connection(pool->acquire()) {}
  ~Lease() {pool->release(std::move(connection));}
};
Json rows(const pqxx::result &r) {
  Json a = Json::array();
  for (auto x : r)
    a.push_back(Json::parse(x[0].c_str()));
  return a;
}
Json one(const pqxx::result &r) {
  if (r.empty())
    throw Error("NOT_FOUND", "Entity not found");
  return Json::parse(r[0][0].c_str());
}
std::string text(const Json &p, const char *k, const std::string &d = "") {
  return p.value(k, d);
}
std::string pattern(const std::string &value) {
  std::string out = "%";
  for (char c : value) {
    if (c == '%' || c == '_' || c == '\\')
      out += '\\';
    out += c;
  }
  return out + "%";
}
int limit(const Json &p) {
  int v = p.value("limit", 50);
  if (v < 1 || v > 200)
    throw std::invalid_argument("limit must be 1..200");
  return v;
}
int offset(const Json &p) {
  int v = p.value("offset", 0);
  if (v < 0)
    throw std::invalid_argument("offset must be nonnegative");
  return v;
}
const std::string resourceSelect = R"SQL(
SELECT to_jsonb(r)||to_jsonb(d)||jsonb_build_object('authors',
 COALESCE((SELECT jsonb_agg(a.name ORDER BY ra.position) FROM resource_authors ra JOIN authors a USING(author_id) WHERE ra.resource_id=r.resource_id),'[]'),
 'vendors',COALESCE((SELECT jsonb_agg(to_jsonb(v)||to_jsonb(o) ORDER BY v.vendor_id) FROM vendor_offers o JOIN vendors v USING(vendor_id) WHERE o.resource_id=r.resource_id),'[]'))
FROM resources r JOIN resource_details d USING(resource_id)
)SQL";
Json getResource(pqxx::work &t, const std::string &id) {
  auto j = one(t.exec_params(resourceSelect + " WHERE r.resource_id=$1", id));
  j["price"] = money(j["unit_price_minor"], j["currency"]);
  return j;
}
void saveResource(pqxx::work &t, const Json &input, bool update = false) {
  Json j = validateResource(input);
  auto id = j.at("resource_id").get<std::string>();
  if (update) {
    getResource(t, id);
    // Historical/financial identity is immutable once referenced. Metadata-only
    // changes remain possible.
    auto old = getResource(t, id);
    if (old["resource_type"] != j["resource_type"] ||
        old["currency"] != j["currency"])
      throw Error("CONFLICT", "Resource type/currency are immutable");
    t.exec_params("UPDATE resources SET "
                  "title=$2,publisher=$3,publication_year=$4,unit_price_minor=$"
                  "5,updated_at=now() WHERE resource_id=$1",
                  id, text(j, "title"), text(j, "publisher"),
                  j.value("publication_year", 2026),
                  j.at("unit_price_minor").get<std::int64_t>());
    t.exec_params("DELETE FROM resource_details WHERE resource_id=$1", id);
    t.exec_params("DELETE FROM resource_authors WHERE resource_id=$1", id);
  } else
    t.exec_params("INSERT INTO "
                  "resources(resource_id,resource_type,category,title,"
                  "publisher,publication_year,currency,unit_price_minor) "
                  "VALUES($1,$2,$3,$4,$5,$6,$7,$8)",
                  id, text(j, "resource_type"), j.at("category").get<int>(),
                  text(j, "title"), text(j, "publisher"),
                  j.value("publication_year", 2026), text(j, "currency"),
                  j.at("unit_price_minor").get<std::int64_t>());
  t.exec_params(
      R"SQL(INSERT INTO resource_details VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18))SQL",
      id, text(j, "isbn"), text(j, "issn"), j.value("edition", 1),
      j.value("binding", 0), j.value("issues_per_year", 1),
      j.value("subscription_years", 1), text(j, "access_url"),
      j.value("license", 1), j.value("platform_fee_minor", std::int64_t(0)),
      text(j, "platform_currency", text(j, "currency")), j.value("format", 0),
      j.value("drm", false), text(j, "narrator"),
      j.value("duration_minutes", 1), text(j, "university"), text(j, "degree"),
      text(j, "supervisor"));
  int pos = 0;
  for (const auto &a : j.value("authors", Json::array())) {
    auto n = a.get<std::string>();
    auto aid =
        t.exec_params("INSERT INTO authors(name) VALUES($1) ON CONFLICT(name) "
                      "DO UPDATE SET name=EXCLUDED.name RETURNING author_id",
                      n)[0][0]
            .as<long long>();
    t.exec_params("INSERT INTO resource_authors VALUES($1,$2,$3)", id, aid,
                  pos++);
  }
}
Budget loadBudget(pqxx::work &t, const Json &j) {
  std::string id = j.at("budget_id"), c = j.at("currency");
  Budget b(Money::fromMinor(j.at("total_minor"), c));
  for (auto r : t.exec_params("SELECT category,max_units,max_spend,max_titles "
                              "FROM budget_quotas WHERE budget_id=$1",
                              id))
    b.setQuota(
        static_cast<ResourceCategory>(r[0].as<int>()),
        {r[1].as<int>(), Money::fromMinor(r[2].as<long long>(), c),
         r[3].is_null() ? std::nullopt : std::optional<int>(r[3].as<int>())});
  PersistenceAccess::spent(b, Money::fromMinor(j.at("spent_minor"), c));
  for (auto r : t.exec_params(
           "SELECT category,units,spent FROM budget_usage WHERE budget_id=$1",
           id))
    PersistenceAccess::usage(b, static_cast<ResourceCategory>(r[0].as<int>()),
                             r[1].as<int>(),
                             Money::fromMinor(r[2].as<long long>(), c));
  for (auto r : t.exec_params("SELECT category,resource_id,units FROM "
                              "budget_title_usage WHERE budget_id=$1",
                              id))
    PersistenceAccess::title(b, static_cast<ResourceCategory>(r[0].as<int>()),
                             r[1].as<std::string>(), r[2].as<int>());
  return b;
}
void saveBudget(pqxx::work &t, const std::string &id, const Budget &b) {
  t.exec_params(
      "UPDATE budgets SET total_minor=$2,spent_minor=$3 WHERE budget_id=$1", id,
      b.total().minorUnits(), b.spent().minorUnits());
  t.exec_params("DELETE FROM budget_usage WHERE budget_id=$1", id);
  t.exec_params("DELETE FROM budget_title_usage WHERE budget_id=$1", id);
  t.exec_params("DELETE FROM budget_quotas WHERE budget_id=$1", id);
  for (int i = 0; i < 4; ++i) {
    auto c = static_cast<ResourceCategory>(i);
    auto u = b.usageFor(c);
    t.exec_params("INSERT INTO budget_usage VALUES($1,$2,$3,$4)", id, i,
                  u.units, u.spent.minorUnits());
    if (auto q = b.quotaFor(c))
      t.exec_params("INSERT INTO budget_quotas VALUES($1,$2,$3,$4,$5)", id, i,
                    q->maxUnits, q->maxSpend.minorUnits(), q->maxTitles);
  }
  for (const auto &[c, ts] : PersistenceAccess::titles(b))
    for (const auto &[rid, n] : ts)
      t.exec_params("INSERT INTO budget_title_usage VALUES($1,$2,$3,$4)", id,
                    static_cast<int>(c), rid, n);
}
void saveRecord(pqxx::work &t, const PurchaseRecord &r,
                const std::string &selection) {
  auto j = record(r);
  j["vendor_selection"] = selection;
  j["resource_id"] =
      t.exec_params("SELECT 1 FROM resources WHERE resource_id=$1",
                    r.resourceId)
              .empty()
          ? Json(nullptr)
          : Json(r.resourceId);
  j["department_id"] =
      r.department.empty() ||
              t.exec_params("SELECT 1 FROM departments WHERE department_id=$1",
                            r.department)
                  .empty()
          ? Json(nullptr)
          : Json(r.department);
  auto fiscal = t.exec_params(
      "SELECT fiscal_year FROM budgets WHERE COALESCE(department_id,'')=$1",
      r.department);
  j["fiscal_year"] =
      fiscal.empty() ? Json(nullptr) : Json(fiscal[0][0].as<int>());
  t.exec_params("INSERT INTO purchase_records SELECT * FROM "
                "jsonb_populate_record(NULL::purchase_records,$1::jsonb)",
                j.dump());
}
void saveLoan(pqxx::work &t, const Loan &l) {
  t.exec_params(
      R"SQL(INSERT INTO loans SELECT * FROM jsonb_populate_record(NULL::loans,$1::jsonb)
 ON CONFLICT(loan_id) DO UPDATE SET returned_quantity=EXCLUDED.returned_quantity,returned_at=EXCLUDED.returned_at,status=EXCLUDED.status)SQL",
      loan(l).dump());
}
Json search(pqxx::work &t, const Json &p) {
  const int from = p.value("from_year", std::numeric_limits<int>::min()),
            to = p.value("to_year", std::numeric_limits<int>::max());
  if (from > to)
    throw std::invalid_argument("invalid year range");
  const std::map<std::string, std::string> sorts = {
      {"id", "r.resource_id"},
      {"title_asc", "lower(r.title),r.resource_id"},
      {"title_desc", "lower(r.title) DESC,r.resource_id"},
      {"year_asc", "r.publication_year,r.resource_id"},
      {"year_desc", "r.publication_year DESC,r.resource_id"},
      {"price_asc", "r.unit_price_minor,r.resource_id"},
      {"price_desc", "r.unit_price_minor DESC,r.resource_id"},
      {"rank",
       "ts_rank(to_tsvector('simple',r.title||' "
       "'||r.publisher),plainto_tsquery('simple',$8)) DESC,r.resource_id"}};
  const auto sort = text(p, "sort", "id");
  if (!text(p,"after").empty() && (sort!="id" || offset(p)!=0 || !text(p,"q").empty()))
    throw std::invalid_argument("Cursor requires id sort and zero offset");
  if (!sorts.count(sort))
    throw std::invalid_argument("invalid sort");
  std::string where =
      R"SQL( WHERE ($1='%%' OR lower(r.title) LIKE lower($1) ESCAPE '\')
 AND ($2='%%' OR EXISTS(SELECT 1 FROM resource_authors ra JOIN authors a USING(author_id) WHERE ra.resource_id=r.resource_id AND lower(a.name) LIKE lower($2) ESCAPE '\'))
 AND ($3='' OR r.resource_id=$3 OR d.isbn=$3 OR d.issn=$3) AND ($4=-1 OR r.category=$4) AND r.publication_year BETWEEN $5 AND $6
 AND ($7='' OR r.currency=$7) AND ($8='' OR r.resource_id=$8 OR d.isbn=$8 OR d.issn=$8 OR lower(r.title) LIKE lower($13) ESCAPE '\' OR lower(r.publisher) LIKE lower($13) ESCAPE '\' OR to_tsvector('simple',r.title||' '||r.publisher) @@ plainto_tsquery('simple',$8)
 OR EXISTS(SELECT 1 FROM resource_authors ra JOIN authors a USING(author_id) WHERE ra.resource_id=r.resource_id AND to_tsvector('simple',a.name) @@ plainto_tsquery('simple',$8))))SQL";
  where += " AND ($11='' OR r.resource_id>$11) AND (NOT $12::boolean OR r.holdings>r.lent)";
  auto query = [&](const std::string &sql) {
    return t.exec_params(sql, pattern(text(p, "title")),
                         pattern(text(p, "author")), text(p, "identifier"),
                         p.value("category", -1), from, to, text(p, "currency"),
                         text(p, "q"), limit(p), offset(p),text(p,"after"),p.value("available_only",false),pattern(text(p,"q")));
  };
  // Check all matching rows, not just one page, before currency-sensitive
  // sorting.
  if (sort.rfind("price_", 0) == 0) {
    auto r = query("SELECT count(DISTINCT currency) FROM resources r JOIN "
                   "resource_details d USING(resource_id)" +
                   where + " AND $9::int>0 AND $10::int>=0");
    if (r[0][0].as<int>() > 1)
      throw std::invalid_argument("cannot sort mixed currencies by price");
  }
  auto result = rows(query(resourceSelect + where + " ORDER BY " +
                           (text(p,"q").empty() ? std::string{} : "CASE WHEN r.resource_id=$8 THEN 0 ELSE 1 END,") + sorts.at(sort) + " LIMIT $9 OFFSET $10"));
  for (auto &r : result)
    r["price"] = money(r["unit_price_minor"], r["currency"]);
  return {{"items", result}, {"limit", limit(p)}, {"offset", offset(p)},
          {"next_cursor", result.size()==static_cast<std::size_t>(limit(p)) && sort=="id" && text(p,"q").empty() ? result.back()["resource_id"] : Json(nullptr)}};
}
Json dispatch(pqxx::work &t, const std::string &op, const Json &p) {
  if (op == "health_check") {
    if (t.exec("SELECT id FROM app_state WHERE id=1").empty())
      throw Error("NOT_CONFIGURED", "Database initialization is incomplete");
    return {{"database", "ok"}, {"cpp_core", "ok"}};
  }
  if (op == "search_resources")
    return search(t, p);
  if (op == "get_resource")
    return getResource(t, p.at("resource_id"));
  if (op == "get_availability") {
    auto j = getResource(t, p.at("resource_id"));
    return {{"resource_id", j["resource_id"]},
            {"holdings", j["holdings"]},
            {"borrowed", j["lent"]},
            {"available", j["holdings"].get<int>() - j["lent"].get<int>()}};
  }
  if (op == "get_events_cursor") {
    auto result=rows(t.exec_params("SELECT to_jsonb(e)||jsonb_build_object('title',r.title) FROM audit_events e LEFT JOIN resources r USING(resource_id) WHERE ($1::bigint=0 OR event_id<$1) ORDER BY event_id DESC LIMIT $2",p.value("before",std::int64_t(0)),limit(p)));
    return {{"items",result},{"next_cursor",result.size()==static_cast<std::size_t>(limit(p)) ? result.back()["event_id"] : Json(nullptr)}};
  }
  if (op == "get_recent_events")
    return rows(
        t.exec_params("SELECT to_jsonb(e)||jsonb_build_object('title',r.title) FROM audit_events e LEFT JOIN resources r USING(resource_id) ORDER BY "
                      "occurred_at DESC,event_id DESC LIMIT $1 OFFSET $2",
                      limit(p), offset(p)));
  if (op == "update_vendor") {
    auto check=resource({{"resource_id","vendor-validation"},{"resource_type","Book"},{"title","Vendor validation"},{"currency","INR"},{"unit_price_minor",0}});
    check->addVendor({text(p,"name"),Money::fromMinor(0,"INR"),text(p,"vendor_id"),p.at("rating").get<double>(),p.at("estimated_delivery_days").get<int>()});
    return one(t.exec_params("UPDATE vendors SET name=$2,rating=$3,estimated_delivery_days=$4 WHERE vendor_id=$1 RETURNING to_jsonb(vendors)",text(p,"vendor_id"),text(p,"name"),p.at("rating").get<double>(),p.at("estimated_delivery_days").get<int>()));
  }
  if (op == "list_users")
    return rows(t.exec_params("SELECT to_jsonb(u) FROM users u ORDER BY user_id LIMIT $1 OFFSET $2",limit(p),offset(p)));
  if (op == "list_departments")
    return rows(t.exec_params("SELECT to_jsonb(d) FROM departments d ORDER BY department_id LIMIT $1 OFFSET $2",limit(p),offset(p)));
  if (op == "list_vendors")
    return rows(t.exec_params("SELECT to_jsonb(v) FROM vendors v ORDER BY vendor_id LIMIT $1 OFFSET $2",limit(p),offset(p)));
  if (op == "list_loans")
    return rows(t.exec_params("SELECT to_jsonb(l)||jsonb_build_object('title',r.title) FROM loans l LEFT JOIN resources r USING(resource_id) ORDER BY borrowed_at DESC,loan_id LIMIT $1 OFFSET $2",limit(p),offset(p)));
  if (op == "list_orders")
    return rows(t.exec_params("SELECT to_jsonb(r)||jsonb_build_object('current_status',CASE WHEN c.order_id IS NULL THEN r.status ELSE 3 END,'cancellation_order_id',c.order_id,'cost',jsonb_build_object('minor_units',r.post_tax_minor,'currency',r.currency)) FROM purchase_records r LEFT JOIN purchase_records c ON c.related_order_id=r.order_id WHERE ($1='' OR r.requested_department=$1) ORDER BY r.order_id DESC LIMIT $2 OFFSET $3",text(p,"department_id"),limit(p),offset(p)));
  if (op == "get_user_loans")
    return rows(
        t.exec_params("SELECT to_jsonb(l)||jsonb_build_object('title',r.title) FROM loans l LEFT JOIN resources r USING(resource_id) WHERE borrower_id=$1 "
                      "ORDER BY loan_id LIMIT $2 OFFSET $3",
                      text(p, "user_id"), limit(p), offset(p)));
  if (op == "get_loan")
    return one(t.exec_params("SELECT to_jsonb(l)||jsonb_build_object('title',r.title) FROM loans l LEFT JOIN resources r USING(resource_id) WHERE loan_id=$1",
                             text(p, "loan_id")));
  if (op == "get_order") {
    auto j = one(t.exec_params(
        "SELECT to_jsonb(r) FROM purchase_records r WHERE order_id=$1",
        p.at("order_id").get<int>()));
    j["current_status"] =
        t.exec_params(
             "SELECT 1 FROM purchase_records WHERE related_order_id=$1",
             p.at("order_id").get<int>())
                .empty()
            ? j["status"]
            : Json(3);
    j["cost"] = money(j["post_tax_minor"], j["currency"]);
    return j;
  }
  if (op == "get_lending_summary") {
    auto r =
        t.exec("SELECT COALESCE(sum(holdings),0),COALESCE(sum(lent),0),(SELECT "
               "count(*) FROM loans WHERE status=0) FROM resources")[0];
    return {{"total_holdings", r[0].as<long long>()},
            {"borrowed", r[1].as<long long>()},
            {"available", r[0].as<long long>() - r[1].as<long long>()},
            {"active_loans", r[2].as<long long>()}};
  }
  if (op == "import_resources") {
    const auto &items = p.at("resources");
    if (!items.is_array() || items.empty() || items.size() > 1000)
      throw std::invalid_argument("Import size must be 1..1000");
    for (const auto &item : items)
      saveResource(t, item);
    return {{"created", items.size()}};
  }
  if (op == "create_resource" || op == "update_resource") {
    const int opening = p.value("initial_holdings", 0);
    if (opening < 0 || (op == "update_resource" && opening != 0))
      throw std::invalid_argument("Opening stock is nonnegative and create-only");
    saveResource(t, p, op == "update_resource");
    if (opening) {
      // Existing owned stock, not a purchase: validate through the domain.
      Catalog openingCatalog;
      openingCatalog.add(resource(p));
      openingCatalog.addHoldings(text(p,"resource_id"), opening);
      t.exec_params("UPDATE resources SET holdings=$2 WHERE resource_id=$1",
                    text(p,"resource_id"), openingCatalog.holdings(text(p,"resource_id")));
    }
    return getResource(t, p.at("resource_id"));
  }
  if (op == "delete_resource") {
    auto r = getResource(t, p.at("resource_id"));
    if (r["holdings"] != 0)
      throw Error("CONFLICT", "Cannot delete held resource");
    t.exec_params("DELETE FROM resources WHERE resource_id=$1",
                  text(p, "resource_id"));
    return {{"deleted", true}};
  }
  if (op == "create_department") {
    requireId(text(p, "department_id"), "department id");
    requireId(text(p, "name"), "name");
    return one(t.exec_params("INSERT INTO departments(department_id,name) "
                             "VALUES($1,$2) RETURNING to_jsonb(departments)",
                             text(p, "department_id"), text(p, "name")));
  }
  if (op == "create_user") {
    requireId(text(p, "user_id"), "user id");
    requireId(text(p, "name"), "name");
    requireId(text(p, "email"), "email");
    return one(t.exec_params(
        "INSERT INTO users(user_id,name,email,role,department_id) "
        "VALUES($1,$2,$3,$4,NULLIF($5,'')) RETURNING to_jsonb(users)",
        text(p, "user_id"), text(p, "name"), text(p, "email"),
        text(p, "role", "Student"), text(p, "department_id")));
  }
  if (op == "get_user")
    return one(t.exec_params("SELECT to_jsonb(u) FROM users u WHERE user_id=$1",
                             text(p, "user_id")));
  if (op == "set_user_active")
    return one(t.exec_params(
        "UPDATE users SET active=$2 WHERE user_id=$1 RETURNING to_jsonb(users)",
        text(p, "user_id"), p.at("active").get<bool>()));
  if (op == "create_budget") {
    Budget b(Money::fromMinor(p.at("total_minor"), text(p, "currency")));
    requireId(text(p, "budget_id"), "budget id");
    t.exec_params("INSERT INTO "
                  "budgets(budget_id,department_id,fiscal_year,currency,total_"
                  "minor) VALUES($1,NULLIF($2,''),$3,$4,$5)",
                  text(p, "budget_id"), text(p, "department_id"),
                  p.at("fiscal_year").get<int>(), b.total().currency(),
                  b.total().minorUnits());
    return budgetSummary(b);
  }
  if (op == "set_offer") {
    auto r = resource(getResource(t, p.at("resource_id")));
    auto v = p; // validate with original Resource rules, avoiding duplicate
                // during update
    auto raw = getResource(t, p.at("resource_id"));
    raw["vendors"] = Json::array();
    r = resource(raw);
    r->addVendor({text(v, "name"),
                  Money::fromMinor(v.at("price_minor"), text(v, "currency")),
                  text(v, "vendor_id"), v.at("rating"),
                  v.at("estimated_delivery_days")});
    // Vendor metadata is global. Conflicting metadata requires an explicit
    // vendor update, not incidental offer replacement.
    auto existing = t.exec_params("SELECT name,rating,estimated_delivery_days "
                                  "FROM vendors WHERE vendor_id=$1",
                                  text(v, "vendor_id"));
    if (!existing.empty() &&
        (existing[0][0].as<std::string>() != text(v, "name") ||
         existing[0][1].as<double>() != v.at("rating").get<double>() ||
         existing[0][2].as<int>() !=
             v.at("estimated_delivery_days").get<int>()))
      throw Error("CONFLICT", "Vendor metadata differs from existing vendor");
    t.exec_params(
        "INSERT INTO vendors VALUES($1,$2,$3,$4) ON CONFLICT DO NOTHING",
        text(v, "vendor_id"), text(v, "name"), v.at("rating").get<double>(),
        v.at("estimated_delivery_days").get<int>());
    t.exec_params("INSERT INTO vendor_offers VALUES($1,$2,$3,$4) ON "
                  "CONFLICT(resource_id,vendor_id) DO UPDATE SET "
                  "price_minor=EXCLUDED.price_minor,currency=EXCLUDED.currency",
                  text(p, "resource_id"), text(v, "vendor_id"),
                  v.at("price_minor").get<std::int64_t>(),
                  r->vendors().front().price.currency());
    return getResource(t, p.at("resource_id"));
  }
  if (op == "configure") {
    Catalog c({p.value("loan_days", 14)});
    Catalog cat;
    Budget b(Money{});
    AcquisitionManager a(
        cat, b, {p.value("print_tax", 0), p.value("electronic_tax", 0)});
    t.exec_params("UPDATE app_state SET "
                  "print_tax=$1,electronic_tax=$2,loan_days=$3 WHERE id=1",
                  a.taxRates().printPercent, a.taxRates().electronicPercent,
                  c.lendingConfig().defaultLoanDays);
    return {{"configured", true}};
  }
  // Construct a transaction-local domain aggregate. No mutable state survives
  // the transaction.
  const auto state =
      t.exec("SELECT next_order,next_loan,print_tax,electronic_tax,loan_days "
             "FROM app_state WHERE id=1")[0];
  Catalog cat({state[4].as<int>()});
  std::map<std::string, Budget> budgets;
  std::map<std::string, Json> budgetRows;
  for (auto j :
       rows(t.exec("SELECT to_jsonb(b) FROM budgets b ORDER BY budget_id"))) {
    std::string key = j["department_id"].is_null()
                          ? ""
                          : j["department_id"].get<std::string>();
    budgets.emplace(key, loadBudget(t, j));
    budgetRows[key] = j;
  }
  if (!budgets.count(""))
    throw Error("NOT_CONFIGURED", "Default budget missing; run seed/setup");
  AcquisitionManager acq(cat, budgets.at(""),
                         {state[2].as<int>(), state[3].as<int>()});
  for (auto &[d, b] : budgets)
    if (!d.empty())
      acq.addDepartment(d, b);
  PersistenceAccess::counters(cat, acq, state[1].as<std::uint64_t>(),
                              state[0].as<int>());
  std::set<std::string> ids;
  if (p.contains("resource_id"))
    ids.insert(p.at("resource_id"));
  if (op == "batch_purchase")
    for (auto q : p.at("requests"))
      ids.insert(q.at("resource_id"));
  if (op == "return_loan") {
    auto l =
        one(t.exec_params("SELECT to_jsonb(l)||jsonb_build_object('title',r.title) FROM loans l LEFT JOIN resources r USING(resource_id) WHERE loan_id=$1",
                          text(p, "loan_id")));
    ids.insert(l["resource_id"]);
  }
  if (op == "cancel_purchase") {
    auto r = one(t.exec_params(
        "SELECT to_jsonb(r) FROM purchase_records r WHERE order_id=$1",
        p.at("order_id").get<int>()));
    ids.insert(r.at("requested_resource_id"));
  }
  for (const auto &id : ids) {
    // Missing resources can be recorded as rejected acquisition requests by the
    // V1 engine.
    if (t.exec_params("SELECT 1 FROM resources WHERE resource_id=$1", id)
            .empty())
      continue;
    auto j = getResource(t, id);
    cat.add(resource(j));
    PersistenceAccess::holdings(cat, id, j["holdings"], j["lent"]);
    for (auto l : rows(t.exec_params(
             "SELECT to_jsonb(l)||jsonb_build_object('title',r.title) FROM loans l LEFT JOIN resources r USING(resource_id) WHERE resource_id=$1", id)))
      PersistenceAccess::restoreLoan(cat, loan(l));
  }
  for (auto j : rows(t.exec(
           "SELECT to_jsonb(r) FROM purchase_records r ORDER BY order_id")))
    PersistenceAccess::history(acq, record(j));
  const auto historySize = acq.history().size();
  Json out;
  auto dept = text(p, "department_id");
  if (!budgets.count(dept))
    throw Error("NOT_FOUND", "Department budget not found");
  auto &b = budgets.at(dept);
  auto mode = [&](const Json &q) {
    const std::map<std::string, VendorSelection> modes = {
        {"Cheapest", VendorSelection::Cheapest},
        {"Fastest", VendorSelection::Fastest},
        {"HighestRated", VendorSelection::HighestRated}};
    auto v = text(q, "vendor_selection", "Cheapest");
    if (!modes.count(v))
      throw std::invalid_argument("invalid vendor selection");
    return modes.at(v);
  };
  if (op == "get_budget_summary" || op == "get_department_budget") {
    auto summary = budgetSummary(b);
    summary["fiscal_year"] = budgetRows.at(dept).at("fiscal_year");
    summary["department_id"] = dept;
    return summary;
  }
  if (op == "get_acquisition_summary") {
    auto s = acq.summary();
    return {{"approved", s.approved},
            {"rejected", s.rejected},
            {"cancelled", s.cancelled},
            {"pre_tax",
             money(s.totalPreTax.minorUnits(), s.totalPreTax.currency())},
            {"post_tax",
             money(s.totalPostTax.minorUnits(), s.totalPostTax.currency())}};
  }
  if (op == "quote_purchase" || op == "check_purchase") {
    auto cost = acq.quote(text(p, "resource_id"), p.at("quantity"), mode(p));
    Json q = {{"cost", money(cost.minorUnits(), cost.currency())}};
    AcquisitionManager untaxed(cat,b,{0,0});
    auto pre=untaxed.quote(text(p,"resource_id"),p.at("quantity"),mode(p));
    q["pre_tax"]=money(pre.minorUnits(),pre.currency());
    auto tax=cost-pre;
    q["tax"]=money(tax.minorUnits(),tax.currency());
    q["title"]=cat.get(text(p,"resource_id")).title();
    q["quantity"]=p.at("quantity");
    q["budget_before"]=budgetSummary(b);
    if (auto v=cat.get(text(p,"resource_id")).selectVendor(mode(p)))
      q["selected_vendor"]={{"vendor_id",v->vendorId},{"name",v->vendor},
                            {"price",money(v->price.minorUnits(),v->price.currency())},{"rating",v->rating},{"estimated_delivery_days",v->deliveryDays}};
    else q["selected_vendor"]=nullptr;
    if (op == "check_purchase") {
      std::string reason;
      try {
        auto simulated = b;
        simulated.commit(cat.get(text(p, "resource_id")).category(),
                         text(p, "resource_id"), p.at("quantity"), cost);
        q["budget_after"]=budgetSummary(simulated);
        if (p.at("quantity").get<int>() >
            std::numeric_limits<int>::max() -
                cat.holdings(text(p, "resource_id")))
          throw std::overflow_error("holdings overflow");
      } catch (const std::bad_alloc &) {
        throw;
      } catch (const std::exception &e) {
        reason = e.what();
      }
      q["allowed"] = reason.empty();
      q["reason"] = reason;
    }
    return q;
  }
  if (op == "set_quota") {
    auto c = p.at("category").get<int>();
    if (c < 0 || c > 3)
      throw std::invalid_argument("invalid category");
    std::optional<int> titles;
    if (p.contains("max_titles") && !p["max_titles"].is_null())
      titles = p["max_titles"].get<int>();
    b.setQuota(static_cast<ResourceCategory>(c),
               {p.at("max_units"),
                Money::fromMinor(p.at("max_spend"), b.total().currency()),
                titles});
    out = budgetSummary(b);
  } else if (op == "rollover") {
    auto row = budgetRows.at(dept);
    int year = p.at("fiscal_year");
    if (year <= row["fiscal_year"].get<int>())
      throw std::invalid_argument("fiscal year must increase");
    // Refunds from a closed year must not debit the next year's usage.
    auto next = b.nextYear(
        Money::fromMinor(p.at("new_allocation"), b.total().currency()),
        p.at("carry_percent"));
    t.exec_params("INSERT INTO "
                  "budget_rollovers(budget_id,from_year,to_year,old_total,old_"
                  "spent,new_total,currency) VALUES($1,$2,$3,$4,$5,$6,$7)",
                  row["budget_id"].get<std::string>(),
                  row["fiscal_year"].get<int>(), year, b.total().minorUnits(),
                  b.spent().minorUnits(), next.total().minorUnits(),
                  b.total().currency());
    b.swap(next);
    t.exec_params("UPDATE budgets SET fiscal_year=$2 WHERE budget_id=$1",
                  row["budget_id"].get<std::string>(), year);
    out = budgetSummary(b);
  } else if (op == "submit_purchase") {
    PurchaseRequest req{text(p, "resource_id"), p.at("quantity"), dept};
    req.vendorSelection = mode(p);
    out = record(acq.submit(req));
  } else if (op == "batch_purchase") {
    std::vector<PurchaseRequest> requests;
    for (auto q : p.at("requests")) {
      PurchaseRequest r{text(q, "resource_id"), q.at("quantity"),
                        text(q, "department_id")};
      r.vendorSelection = mode(q);
      requests.push_back(r);
    }
    out = Json::array();
    for (auto r : acq.processBatch(requests, p.value("atomic", true)))
      out.push_back(record(r));
  } else if (op == "cancel_purchase") {
    auto order = p.at("order_id").get<int>();
    auto r = one(t.exec_params(
        "SELECT to_jsonb(r) FROM purchase_records r WHERE order_id=$1", order));
    auto fiscal = t.exec_params(
        "SELECT fiscal_year FROM budgets WHERE COALESCE(department_id,'')=$1",
        r.at("requested_department").get<std::string>());
    if (!r["fiscal_year"].is_null() && !fiscal.empty() &&
        r["fiscal_year"].get<int>() != fiscal[0][0].as<int>())
      throw Error("CONFLICT", "Cannot refund a closed fiscal year");
    out = record(acq.cancel(order));
  } else if (op == "borrow_resource") {
    auto u =
        one(t.exec_params("SELECT to_jsonb(u) FROM users u WHERE user_id=$1",
                          text(p, "borrower_id")));
    if (!u["active"].get<bool>())
      throw Error("CONFLICT", "Borrower inactive");
    auto when = p.contains("borrowed_at") ? timestamp(p.at("borrowed_at"))
                                          : std::chrono::system_clock::now();
    std::optional<Timestamp> due;
    if (p.contains("due_at"))
      due = timestamp(p.at("due_at"));
    out = loan(cat.borrow(text(p, "resource_id"), text(p, "borrower_id"),
                          p.at("quantity"), when, due));
  } else if (op == "return_loan") {
    cat.returnBorrowed(text(p, "loan_id"), p.at("quantity"),
                       p.contains("returned_at")
                           ? timestamp(p.at("returned_at"))
                           : std::chrono::system_clock::now());
    out = loan(cat.loan(text(p, "loan_id")));
  } else
    throw Error("UNKNOWN_OPERATION", "Unknown application operation");
  if (op == "borrow_resource" || op == "return_loan")
    out["title"] = cat.get(text(out,"resource_id")).title();
  for (auto &[d, value] : budgets)
    saveBudget(t, budgetRows.at(d).at("budget_id"), value);
  for (auto *r : cat.all())
    t.exec_params("UPDATE resources SET holdings=$2,lent=$3,updated_at=now() "
                  "WHERE resource_id=$1",
                  r->id(), cat.holdings(r->id()), cat.lent(r->id()));
  for (const auto &[id, l] : PersistenceAccess::loans(cat))
    saveLoan(t, l);
  for (std::size_t i = historySize; i < acq.history().size(); ++i) {
    std::string selection = text(p, "vendor_selection", "Cheapest");
    if (op == "batch_purchase")
      selection = text(p.at("requests").at(i - historySize), "vendor_selection",
                       "Cheapest");
    if (op == "cancel_purchase")
      selection = t.exec_params("SELECT vendor_selection FROM purchase_records "
                                "WHERE order_id=$1",
                                p.at("order_id").get<int>())[0][0]
                      .as<std::string>();
    saveRecord(t, acq.history()[i], selection);
  }
  for (const auto &e : cat.events())
    t.exec_params("INSERT INTO "
                  "audit_events(event_type,resource_id,quantity,actor,user_id,"
                  "department_id,reference_id,"
                  "occurred_at) VALUES($1,$2,$3,$4,CASE WHEN $1 IN(2,3) THEN "
                  "NULLIF($4,'') END,CASE WHEN $1 IN(0,1) THEN NULLIF($4,'') "
                  "END,$5,$6::timestamptz)",
                  static_cast<int>(e.type), e.resourceId, e.quantity, e.actor,
                  e.referenceId, timestamp(e.timestamp));
  t.exec_params("UPDATE app_state SET next_order=$1,next_loan=$2 WHERE id=1",
                PersistenceAccess::nextOrder(acq),
                PersistenceAccess::nextLoan(cat));
  return out;
}
} // namespace
PostgresRepository::PostgresRepository(std::string c)
    : connection_(std::move(c)) {
  if (connection_.empty())
    throw Error("CONFIGURATION", "Database URL is required");
  if (connection_.find("connect_timeout=")==std::string::npos) {
    if (connection_.rfind("postgres",0)==0)
      connection_+=(connection_.find('?')==std::string::npos ? "?" : "&")+std::string("connect_timeout=5");
    else connection_+=" connect_timeout=5";
  }
  pool_=std::make_shared<ConnectionPool>(connection_);
}
Json PostgresRepository::execute(const std::string &op, const Json &p) {
  try {
    validateNumericInputs(p);
    Lease lease(pool_);
    pqxx::work t(*lease.connection);
    if (op == "search_resources")
      t.exec("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ");
    t.exec("SET LOCAL TIME ZONE 'UTC'");
    t.exec("SET LOCAL lock_timeout='10s'");
    t.exec("SET LOCAL statement_timeout='30s'");
    // A single database row serializes aggregate changes across processes.
    // Explicitly favor correctness over write throughput in V2. Search uses
    // SQL-only snapshots and does not acquire the write lock.
    if (op != "search_resources" && op != "health_check")
      t.exec("SELECT id FROM app_state WHERE id=1 FOR UPDATE");
    auto key = text(p, "request_id");
    Json canonical = p;
    canonical.erase("request_id");
    if (!key.empty()) {
      auto old = t.exec_params("SELECT operation,parameters,result FROM "
                               "acquisition_requests WHERE request_id=$1",
                               key);
      if (!old.empty()) {
        if (old[0][0].as<std::string>() != op ||
            Json::parse(old[0][1].c_str()) != canonical)
          throw Error("CONFLICT",
                      "Request ID reused with different parameters");
        auto result = Json::parse(old[0][2].c_str());
        t.commit();
        return result;
      }
    }
    Json result = dispatch(t, op, p);
    if (!key.empty())
      t.exec_params("INSERT INTO "
                    "acquisition_requests(request_id,operation,parameters,"
                    "result) VALUES($1,$2,$3::jsonb,$4::jsonb)",
                    key, op, canonical.dump(), result.dump());
    t.commit();
    return result;
  } catch (const Error &) {
    throw;
  } catch (const pqxx::unique_violation &) {
    throw Error("CONFLICT", "Duplicate entity or request");
  } catch (const pqxx::integrity_constraint_violation &) {
    throw Error("CONFLICT", "Relational constraint rejected the operation");
  } catch (const pqxx::failure &) {
    throw Error("DATABASE_ERROR", "Database operation failed");
  } catch (const NotFoundError &) {
    throw Error("NOT_FOUND", "Resource not found");
  } catch (const QuotaExceededError &e) {
    throw Error("QUOTA_EXCEEDED", e.what());
  } catch (const BudgetExceededError &e) {
    throw Error("BUDGET_EXCEEDED", e.what());
  } catch (const Json::exception &) {
    throw Error("INVALID_INPUT", "Invalid operation parameters");
  } catch (const std::invalid_argument &e) {
    throw Error("INVALID_INPUT", e.what());
  } catch (const std::overflow_error &) {
    throw Error("OVERFLOW", "Domain numeric capacity exceeded");
  }
}
} // namespace libracore
