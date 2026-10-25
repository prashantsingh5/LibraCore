#include "libracore/Mapping.h"
#include <cassert>
#include <iostream>
using namespace libracore;
int main() {
  for (const auto *type : {"Book", "Journal", "ElectronicResource", "EBook",
                           "AudioBook", "Thesis"}) {
    Json j = {{"resource_id", type}, {"resource_type", type},
              {"title", "Test"},     {"unit_price_minor", 1000},
              {"currency", "inr"},   {"authors", {"A", "B"}}};
    auto r = resource(j);
    assert(r->id() == type);
    assert(r->costFor(1).minorUnits() == 1000);
    assert(validateResource(j)["currency"] == "INR");
  }
  assert(money(-1, "usd")["formatted"] == "USD -0.01");
  auto t = timestamp(std::string("2026-01-01T00:00:00Z"));
  assert(timestamp(t) == "2026-01-01T00:00:00Z");
  bookmgmt::Catalog cat;
  cat.add(resource({{"resource_id", "B"},
                    {"resource_type", "Book"},
                    {"title", "T"},
                    {"unit_price_minor", 100},
                    {"currency", "INR"}}));
  cat.addHoldings("B", 2);
  auto l = cat.borrow("B", "U", 2, t);
  assert(loan(loan(l)).quantity == 2);
  bookmgmt::Budget b(bookmgmt::Money::of(100));
  bookmgmt::AcquisitionManager a(cat, b);
  auto r = a.purchase("B", 1);
  assert(record(record(r)).cost == r.cost);
  std::cout << "Mapping round trips and six resource constructions passed\n";
}
