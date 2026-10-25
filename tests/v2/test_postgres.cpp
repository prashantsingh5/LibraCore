#include "libracore/Repository.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
int main() {
  const char *url = std::getenv("TEST_DATABASE_URL");
  if (!url) {
    std::cerr << "TEST_DATABASE_URL required; run test_v2.sh\n";
    return 1;
  }
  libracore::PostgresRepository repository(url);
  auto r = repository.execute("get_resource", {{"resource_id", "B001"}});
  assert(r["resource_type"] == "Book");
  auto q = repository.execute("quote_purchase",
                              {{"resource_id", "B001"}, {"quantity", 1}});
  assert(q["cost"]["minor_units"] == 9000);
  auto availability =
      repository.execute("get_availability", {{"resource_id", "B001"}});
  try {
    repository.execute("borrow_resource", {{"resource_id", "B001"},
                                           {"borrower_id", "U001"},
                                           {"quantity", 999999}});
    assert(false);
  } catch (const libracore::Error &e) {
    assert(e.code == "INVALID_INPUT");
  }
  assert(repository.execute("get_availability", {{"resource_id", "B001"}}) ==
         availability);
  std::cout << "C++ PostgreSQL reconstruction, pricing and rejected-write "
               "invariants passed\n";
}
