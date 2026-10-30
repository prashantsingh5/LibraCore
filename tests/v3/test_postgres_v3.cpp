#include "libracore/Repository.h"
#include <cassert>
#include <cstdlib>
#include <future>
#include <iostream>
#include <vector>
int main() {
  const char* url=std::getenv("TEST_DATABASE_URL");
  if (!url) {std::cerr<<"TEST_DATABASE_URL required\n";return 2;}
  libracore::PostgresRepository repo(url);
  auto first=repo.execute("search_resources",{{"limit",2}});
  assert(first["items"].size()==2);
  auto second=repo.execute("search_resources",{{"limit",2},{"after",first["next_cursor"]}});
  for (const auto& row:second["items"])
    assert(row["resource_id"].get<std::string>()>first["next_cursor"].get<std::string>());
  bool invalid=false;
  try {repo.execute("search_resources",{{"after","B001"},{"sort","title_asc"}});}
  catch (const libracore::Error& e) {invalid=e.code=="INVALID_INPUT";}
  assert(invalid);
  auto quote=repo.execute("check_purchase",{{"resource_id","B001"},{"quantity",1}});
  assert(quote["pre_tax"]["minor_units"].get<long long>()+quote["tax"]["minor_units"].get<long long>()==quote["cost"]["minor_units"].get<long long>());
  assert(quote.contains("budget_before"));
  std::vector<std::future<bool>> tasks;
  for(int i=0;i<12;++i) tasks.push_back(std::async(std::launch::async,[&repo]{return repo.execute("health_check",libracore::Json::object())["database"]=="ok";}));
  for(auto& task:tasks) assert(task.get());
  auto events=repo.execute("get_events_cursor",{{"limit",2}});
  assert(events["items"].size()<=2);
  std::cout<<"V3 cursor, quote and pooled concurrency checks passed\n";
}
