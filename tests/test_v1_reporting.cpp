#include "TestSupport.h"
#include <limits>
void testV1Reports() {
    Budget b(Money::of(1000));
    assert(b.summary().percentageUsed == 0);
    auto empty = b.categorySummary(ResourceCategory::Book);
    assert(!empty.unitUtilization && !empty.spendUtilization && !empty.titleUtilization);
    b.setQuota(ResourceCategory::Book, {10,Money::of(500),2});
    b.commit(ResourceCategory::Book,"B",2,Money::of(100));
    auto s = b.categorySummary(ResourceCategory::Book);
    assert(s.unitsUsed == 2 && s.unitsRemaining == 8 && s.titlesUsed == 1 && s.titlesRemaining == 1);
    assert(s.spend == Money::of(100) && s.spendRemaining == Money::of(400));
    assert(s.unitUtilization == 20 && s.titleUtilization == 50 && s.spendUtilization == 20);
    assert(b.summary().percentageUsed == 10 && b.summary().remaining == Money::of(900));
    assertThrows<std::invalid_argument>([&]{b.setQuota(ResourceCategory::Book,{0,Money{},0});});
    Budget zero(Money{}); zero.setQuota(ResourceCategory::Book,{0,Money{},0});
    assert(zero.summary().percentageUsed == 0);
    assert(zero.categorySummary(ResourceCategory::Book).unitUtilization == 0);
    assert(zero.categorySummary(ResourceCategory::Book).titleUtilization == 0);
    assert(zero.categorySummary(ResourceCategory::Book).spendUtilization == 0);
    Catalog c; AcquisitionManager a(c, zero); a.addDepartment("CS",b);
    assert(a.summary().approved == 0 && a.summary().totalPostTax == Money{});
    auto ds = a.departmentSummaries(); assert(ds.size()==2 && ds[1].department=="CS" && ds[1].spent==Money::of(100));
    assertThrows<std::invalid_argument>([&]{b.commit(ResourceCategory::Book," ",1,Money{});});
    b.refund(ResourceCategory::Book,"B",2,Money::of(100));
    assert(b.categorySummary(ResourceCategory::Book).titlesUsed == 0);
}
void testV1Money() {
    assert(Money::of(1250).toCurrencyString() == "INR 1250.00");
    assert(Money::of(120,50,"usd").toCurrencyString() == "USD 120.50");
    assert(Money::of(80,0,"eur").toCurrencyString() == "EUR 80.00");
    const auto max = std::numeric_limits<std::int64_t>::max();
    const auto min = std::numeric_limits<std::int64_t>::min();
    assert(Money::fromMinor(min).toString() == "-92233720368547758.08");
    auto m = Money::fromMinor(max);
    assertThrows<std::overflow_error>([&]{ m += Money::fromMinor(1); });
    assert(m.minorUnits() == max);
    assertThrows<std::overflow_error>([&]{ m *= 2; });
    assertThrows<std::overflow_error>([&]{ Money::of(max); });
    assertThrows<std::overflow_error>([&]{ auto x = Money::fromMinor(min); x *= -1; });
    assertThrows<std::overflow_error>([&]{ auto x = Money::fromMinor(min); x -= Money::fromMinor(1); });
    assert(m.scaledRatio(100,100) == m);
    assert(Money::fromMinor(3).scaledRatio(1,2).minorUnits()==1);
    assertThrows<std::invalid_argument>([&]{ m.scaledRatio(1,0); });
}
