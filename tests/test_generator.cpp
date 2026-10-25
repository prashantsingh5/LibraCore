#include "TestSupport.h"
#include "../benchmarks/SyntheticCatalog.h"
#include <set>
void testGenerator() {
    assert(benchmark::generateCatalog(0).empty());
    auto c = benchmark::generateCatalog(1003);
    assert(c.size() == 1003);
    std::set<std::string> ids;
    for (const auto* r : c.all()) {
        assert(ids.insert(r->id()).second);
        assert(r->year() >= 1980 && r->year() <= 2026 && !r->unitPrice().isNegative());
    }
    assert(c.byCategory(ResourceCategory::Book).size() == 335);
    assert(c.byCategory(ResourceCategory::Journal).size() == 334);
    assert(c.byCategory(ResourceCategory::ElectronicResource).size() == 334);
    assert(c.contains(benchmark::syntheticId(1002)));
}
