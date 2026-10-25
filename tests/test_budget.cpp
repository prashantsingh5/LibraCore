#include "TestSupport.h"

void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});
    b.setQuota(ResourceCategory::Journal, {3, Money::of(600)});
    b.setQuota(ResourceCategory::EBook, {5, Money::of(1000)});
    assert(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    assert(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    assert(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    assert(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    assert(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota
    assert(b.check(ResourceCategory::Journal, 2, Money::of(400)).empty());
    assert(!b.check(ResourceCategory::Journal, 4, Money::of(400)).empty());
    assert(b.check(ResourceCategory::EBook, 3, Money::of(800)).empty());
    assert(!b.check(ResourceCategory::EBook, 6, Money::of(800)).empty());
    b.commit(ResourceCategory::Book, 4, Money::of(300));
    assert(b.spent() == Money::of(300));
    assert(*b.unitsRemaining(ResourceCategory::Book) == 1);
    assert(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    assert(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    assertThrows<QuotaExceededError>([&]() {
        (void)(b.commit(ResourceCategory::Book, 2, Money::of(10)));
    });
    assertThrows<BudgetExceededError>([&]() {
        (void)(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800)));
    });
    assertThrows<std::invalid_argument>([&]() {
        (void)(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1)));
    });
    assert(b.spent() == Money::of(300));  // failed commits changed nothing
    std::ostringstream os;
    b.print(os);
    assert(os.str().find("Journal") != std::string::npos);
    assert(os.str().find("EBook") != std::string::npos);
}


void testTitleQuota() {
    Catalog c;

    c.emplace<Book>("B1", "Title One",
                    std::vector<std::string>{"Author"},
                    "isbn-1", "P", 2026, Money::of(100));

    c.emplace<Book>("B2", "Title Two",
                    std::vector<std::string>{"Author"},
                    "isbn-2", "P", 2026, Money::of(100));

    c.emplace<Book>("B3", "Title Three",
                    std::vector<std::string>{"Author"},
                    "isbn-3", "P", 2026, Money::of(100));

    Budget b(Money::of(5000));

    // Max 20 units, max 5000 spend, but only 2 different Book titles.
    b.setQuota(ResourceCategory::Book,
               {20, Money::of(5000), 2});

    AcquisitionManager acq(c, b);

    assert(b.titlesUsed(ResourceCategory::Book) == 0);
    assert(*b.titlesRemaining(ResourceCategory::Book) == 2);

    // First distinct title.
    const auto& r1 = acq.purchase("B1", 2);
    assert(r1.approved);
    assert(b.titlesUsed(ResourceCategory::Book) == 1);
    assert(*b.titlesRemaining(ResourceCategory::Book) == 1);

    // Buying B1 again must NOT consume another title slot.
    const auto& r2 = acq.purchase("B1", 1);
    assert(r2.approved);
    assert(b.titlesUsed(ResourceCategory::Book) == 1);
    assert(*b.titlesRemaining(ResourceCategory::Book) == 1);

    // Second distinct title.
    const auto& r3 = acq.purchase("B2", 1);
    assert(r3.approved);
    assert(b.titlesUsed(ResourceCategory::Book) == 2);
    assert(*b.titlesRemaining(ResourceCategory::Book) == 0);

    // Existing title is still allowed even when title quota is full.
    assert(acq.canPurchase("B1", 1));

    // A third different title must be rejected.
    std::string why;
    assert(!acq.canPurchase("B3", 1, &why));
    assert(why.find("title quota exceeded") != std::string::npos);

    assertThrows<QuotaExceededError>([&]() {
        (void)(acq.purchase("B3", 1));
    });

    // Rejected purchase must not change title usage.
    assert(b.titlesUsed(ResourceCategory::Book) == 2);
    assert(c.holdings("B3") == 0);

    // Check Budget report includes title quota information.
    std::ostringstream os;
    b.print(os);
    assert(os.str().find("Titles used/max") != std::string::npos);
    assert(os.str().find("2/2") != std::string::npos);

    // Negative title limits are invalid.
    assertThrows<std::invalid_argument>([&]() {
        (void)(b.setQuota(ResourceCategory::Journal,
                   {10, Money::of(1000), -1}));
    });
}


void testYearEndRollover() {
    Budget current(Money::of(1000));

    current.setQuota(
        ResourceCategory::Book,
        {10, Money::of(800), 3});

    current.setQuota(
        ResourceCategory::ElectronicResource,
        {20, Money::of(900), 4});

    // Simulate current-year spending and title usage.
    current.commit(
        ResourceCategory::Book,
        "B1",
        2,
        Money::of(200));

    current.commit(
        ResourceCategory::Book,
        "B2",
        1,
        Money::of(100));

    assert(current.spent() == Money::of(300));
    assert(current.remaining() == Money::of(700));
    assert(current.usageFor(ResourceCategory::Book).units == 3);
    assert(current.titlesUsed(ResourceCategory::Book) == 2);

    // Carry forward 50% of 700 = 350.
    // New allocation = 2000, therefore total = 2350.
    Budget next =
        current.nextYear(Money::of(2000), 50);

    assert(next.total() == Money::of(2350));
    assert(next.spent() == Money::of(0));
    assert(next.remaining() == Money::of(2350));

    // Usage must reset in the new year.
    assert(next.usageFor(ResourceCategory::Book).units == 0);
    assert(next.usageFor(ResourceCategory::Book).spent == Money::of(0));
    assert(next.titlesUsed(ResourceCategory::Book) == 0);

    // Quota policy must be preserved.
    auto bookQuota =
        next.quotaFor(ResourceCategory::Book);

    assert(bookQuota.has_value());
    assert(bookQuota->maxUnits == 10);
    assert(bookQuota->maxSpend == Money::of(800));
    assert(bookQuota->maxTitles.has_value());
    assert(*bookQuota->maxTitles == 3);

    auto electronicQuota =
        next.quotaFor(ResourceCategory::ElectronicResource);

    assert(electronicQuota.has_value());
    assert(electronicQuota->maxUnits == 20);
    assert(electronicQuota->maxSpend == Money::of(900));
    assert(electronicQuota->maxTitles.has_value());
    assert(*electronicQuota->maxTitles == 4);

    // 0% carry-forward.
    Budget noCarry =
        current.nextYear(Money::of(2000), 0);

    assert(noCarry.total() == Money::of(2000));

    // 100% carry-forward.
    Budget fullCarry =
        current.nextYear(Money::of(2000), 100);

    assert(fullCarry.total() == Money::of(2700));

    // Verify that next year's budget is independent.
    next.commit(
        ResourceCategory::Book,
        "B3",
        1,
        Money::of(100));

    assert(next.spent() == Money::of(100));
    assert(current.spent() == Money::of(300));

    assert(next.titlesUsed(ResourceCategory::Book) == 1);
    assert(current.titlesUsed(ResourceCategory::Book) == 2);

    // Invalid carry percentages.
    assertThrows<std::invalid_argument>([&]() {
        (void)(current.nextYear(Money::of(1000), -1));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(current.nextYear(Money::of(1000), 101));
    });

    // New allocation may not be negative.
    assertThrows<std::invalid_argument>([&]() {
        (void)(current.nextYear(Money::of(-1), 50));
    });

    // Department budgets can be rolled independently.
    Budget csCurrent(Money::of(1000));
    Budget physicsCurrent(Money::of(2000));

    csCurrent.commit(
        ResourceCategory::Book,
        "CS1",
        1,
        Money::of(400));

    physicsCurrent.commit(
        ResourceCategory::Book,
        "PH1",
        1,
        Money::of(500));

    Budget csNext =
        csCurrent.nextYear(Money::of(1500), 50);

    Budget physicsNext =
        physicsCurrent.nextYear(Money::of(2500), 25);

    // CS remaining = 600, 50% = 300.
    assert(csNext.total() == Money::of(1800));

    // Physics remaining = 1500, 25% = 375.
    assert(physicsNext.total() == Money::of(2875));

    assert(csNext.spent() == Money::of(0));
    assert(physicsNext.spent() == Money::of(0));
}

