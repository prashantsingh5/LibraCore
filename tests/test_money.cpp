#include "TestSupport.h"

void testMoney() {
    assert(Money::of(12, 5).toString() == "12.05");
    assert(Money::of(-3, 50).toString() == "-3.50");
    assert(Money::fromMinor(7).toString() == "0.07");
    assert(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    assert(Money::of(3) * 4 == Money::of(12));
    assert(Money::of(1) < Money::of(2));
    assertThrows<std::invalid_argument>([&]() {
        (void)(Money::of(1, 100));
    });
}


void testCurrencyAwareMoney() {
    // -------------------------------------------------
    // Basic currency storage and normalization
    // -------------------------------------------------

    Money inr = Money::of(100, 50, "INR");
    Money usd = Money::of(10, 25, "USD");
    Money eur = Money::fromMinor(1234, "EUR");

    assert(inr.currency() == "INR");
    assert(usd.currency() == "USD");
    assert(eur.currency() == "EUR");

    assert(inr.toString() == "100.50");
    assert(usd.toString() == "10.25");
    assert(eur.toString() == "12.34");

    // Lowercase input should normalize to uppercase.
    Money lowerUsd = Money::of(5, 0, "usd");

    assert(lowerUsd.currency() == "USD");

    // Default currency remains INR for backward compatibility.
    Money defaultMoney = Money::of(20);

    assert(defaultMoney.currency() == "INR");

    Money defaultZero;

    assert(defaultZero.currency() == "INR");
    assert(defaultZero == Money::of(0));

    // -------------------------------------------------
    // Same-currency arithmetic
    // -------------------------------------------------

    Money usdA = Money::of(100, 0, "USD");
    Money usdB = Money::of(25, 50, "USD");

    Money usdSum = usdA + usdB;

    assert(usdSum.currency() == "USD");
    assert(usdSum == Money::of(125, 50, "USD"));

    Money usdDifference = usdA - usdB;

    assert(usdDifference.currency() == "USD");
    assert(usdDifference == Money::of(74, 50, "USD"));

    Money usdScaled = usdB * 2;

    assert(usdScaled.currency() == "USD");
    assert(usdScaled == Money::of(51, 0, "USD"));

    Money usdScaledReverse = 3 * usdB;

    assert(usdScaledReverse.currency() == "USD");
    assert(usdScaledReverse == Money::of(76, 50, "USD"));

    Money usdMutable = Money::of(10, 0, "USD");

    usdMutable += Money::of(5, 0, "USD");
    assert(usdMutable == Money::of(15, 0, "USD"));

    usdMutable -= Money::of(2, 0, "USD");
    assert(usdMutable == Money::of(13, 0, "USD"));

    // -------------------------------------------------
    // Same-currency comparisons
    // -------------------------------------------------

    assert(Money::of(10, 0, "USD") <
          Money::of(20, 0, "USD"));

    assert(Money::of(10, 0, "USD") <=
          Money::of(10, 0, "USD"));

    assert(Money::of(20, 0, "USD") >
          Money::of(10, 0, "USD"));

    assert(Money::of(20, 0, "USD") >=
          Money::of(20, 0, "USD"));

    assert(Money::of(20, 0, "USD") ==
          Money::of(20, 0, "USD"));

    assert(Money::of(20, 0, "USD") !=
          Money::of(21, 0, "USD"));

    // -------------------------------------------------
    // Mixed-currency arithmetic must throw
    // -------------------------------------------------

    Money testUsd = Money::of(100, 0, "USD");
    Money testInr = Money::of(100, 0, "INR");

    assertThrows<std::invalid_argument>([&]() {
        (void)(testUsd + testInr);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(testUsd - testInr);
    });

    Money mutableUsd = Money::of(100, 0, "USD");

    assertThrows<std::invalid_argument>([&]() {
        (void)(mutableUsd += testInr);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(mutableUsd -= testInr);
    });

    // -------------------------------------------------
    // Mixed-currency comparisons must also throw
    // -------------------------------------------------

    assertThrows<std::invalid_argument>([&]() {
        (void)(testUsd == testInr);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(testUsd != testInr);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(testUsd < testInr);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(testUsd <= testInr);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(testUsd > testInr);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(testUsd >= testInr);
    });

    // -------------------------------------------------
    // Invalid currency codes
    // -------------------------------------------------

    assertThrows<std::invalid_argument>([&]() {
        (void)(Money::of(10, 0, ""));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(Money::of(10, 0, "US"));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(Money::of(10, 0, "USDD"));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(Money::of(10, 0, "U1D"));
    });

    // -------------------------------------------------
    // Budget with non-INR currency
    // -------------------------------------------------

    Budget usdBudget(
        Money::of(1000, 0, "USD"));

    assert(usdBudget.total().currency() == "USD");
    assert(usdBudget.spent().currency() == "USD");
    assert(usdBudget.remaining().currency() == "USD");

    assert(usdBudget.spent() ==
          Money::of(0, 0, "USD"));

    usdBudget.setQuota(
        ResourceCategory::Book,
        {
            10,
            Money::of(500, 0, "USD"),
            5
        });

    assert(
        usdBudget.usageFor(
            ResourceCategory::Book)
            .spent.currency() == "USD");

    usdBudget.commit(
        ResourceCategory::Book,
        "USD-B1",
        2,
        Money::of(200, 0, "USD"));

    assert(usdBudget.spent() ==
          Money::of(200, 0, "USD"));

    assert(usdBudget.remaining() ==
          Money::of(800, 0, "USD"));

    assert(
        usdBudget.usageFor(
            ResourceCategory::Book)
            .spent ==
        Money::of(200, 0, "USD"));

    // Wrong-currency purchase must be rejected.
    assertThrows<std::invalid_argument>([&]() {
        (void)(usdBudget.commit(
            ResourceCategory::Book,
            "USD-B2",
            1,
            Money::of(50, 0, "INR")));
    });

    // Wrong-currency quota must also be rejected.
    assertThrows<std::invalid_argument>([&]() {
        (void)(usdBudget.setQuota(
            ResourceCategory::Journal,
            {
                5,
                Money::of(200, 0, "INR"),
                2
            }));
    });

    // -------------------------------------------------
    // Q10 rollover must preserve currency
    // -------------------------------------------------

    Budget usdNext =
        usdBudget.nextYear(
            Money::of(2000, 0, "USD"),
            50);

    // Current remaining = 800 USD
    // 50% carried = 400 USD
    // New allocation = 2000 USD
    // Next total = 2400 USD
    assert(usdNext.total() ==
          Money::of(2400, 0, "USD"));

    assert(usdNext.total().currency() == "USD");
    assert(usdNext.spent() ==
          Money::of(0, 0, "USD"));

    // Rollover allocation in wrong currency must fail
    // when it is combined with the carry amount.
    assertThrows<std::invalid_argument>([&]() {
        (void)(usdBudget.nextYear(
            Money::of(2000, 0, "EUR"),
            50));
    });

    // -------------------------------------------------
    // Resource pricing preserves currency
    // -------------------------------------------------

    Book usdBook(
        "CUR-B1",
        "USD Hardcover",
        {"Author"},
        "currency-isbn",
        "Publisher",
        2026,
        Money::of(100, 0, "USD"),
        1,
        Binding::Hardcover);

    // 100 * 2 + 20% = 240 USD
    assert(
        usdBook.costFor(2) ==
        Money::of(240, 0, "USD"));

    // Bulk:
    // 100 * 10 = 1000
    // +20% = 1200
    // -10% = 1080 USD
    assert(
        usdBook.costFor(10) ==
        Money::of(1080, 0, "USD"));

    ElectronicResource usdElectronic(
        "CUR-R1",
        "USD Database",
        "Publisher",
        2026,
        Money::of(10, 0, "USD"),
        "https://currency.example",
        LicenseModel::AnnualSubscription,
        Money::of(50, 0, "USD"));

    // 50 platform + 5*10 = 100 USD
    assert(
        usdElectronic.costFor(5) ==
        Money::of(100, 0, "USD"));

    // 50 full seats = 500
    // 10 half-price seats = 50
    // platform = 50
    // total = 600 USD
    assert(
        usdElectronic.costFor(60) ==
        Money::of(600, 0, "USD"));

    // -------------------------------------------------
    // Full USD acquisition workflow
    // -------------------------------------------------

    Catalog currencyCatalog;

    auto& currencyBook =
        currencyCatalog.emplace<Book>(
            "CBUSD",
            "Currency Demo Book",
            std::vector<std::string>{"Author"},
            "usd-demo-isbn",
            "Publisher",
            2026,
            Money::of(120, 0, "USD"));

    currencyBook.addVendor(
        "Vendor A",
        Money::of(100, 0, "USD"));

    currencyBook.addVendor(
        "Vendor B",
        Money::of(80, 0, "USD"));

    Budget currencyBudget(
        Money::of(1000, 0, "USD"));

    currencyBudget.setQuota(
        ResourceCategory::Book,
        {
            10,
            Money::of(800, 0, "USD"),
            5
        });

    AcquisitionManager currencyAcq(
        currencyCatalog,
        currencyBudget,
        TaxRates{10, 20});

    // Cheapest vendor = 80 USD
    // 2 copies = 160 USD
    // 10% tax = 16 USD
    // post-tax = 176 USD
    assert(
        currencyAcq.quote("CBUSD", 2) ==
        Money::of(176, 0, "USD"));

    const PurchaseRecord& usdRecord =
        currencyAcq.purchase("CBUSD", 2);

    assert(usdRecord.vendor == "Vendor B");

    assert(
        usdRecord.preTaxCost ==
        Money::of(160, 0, "USD"));

    assert(
        usdRecord.cost ==
        Money::of(176, 0, "USD"));

    assert(
        currencyBudget.spent() ==
        Money::of(176, 0, "USD"));

    assert(
        currencyBudget.remaining() ==
        Money::of(824, 0, "USD"));

    assert(
        currencyAcq.totalPreTax() ==
        Money::of(160, 0, "USD"));

    assert(
        currencyAcq.totalSpent() ==
        Money::of(176, 0, "USD"));

    // -------------------------------------------------
    // Mixed-currency acquisition should be rejected
    // -------------------------------------------------

    Catalog mixedCatalog;

    mixedCatalog.emplace<Book>(
        "MIX1",
        "INR Book",
        std::vector<std::string>{"Author"},
        "mix-isbn",
        "Publisher",
        2026,
        Money::of(100, 0, "INR"));

    Budget mixedBudget(
        Money::of(1000, 0, "USD"));

    mixedBudget.setQuota(
        ResourceCategory::Book,
        {
            10,
            Money::of(1000, 0, "USD"),
            5
        });

    AcquisitionManager mixedAcq(
        mixedCatalog,
        mixedBudget);

    std::string reason;

    assert(
        !mixedAcq.canPurchase(
            "MIX1",
            1,
            &reason));

    assert(
        reason.find("currency") !=
        std::string::npos);

    assertThrows<std::invalid_argument>([&]() {
        (void)(mixedAcq.purchase("MIX1", 1));
    });

    assert(mixedCatalog.holdings("MIX1") == 0);
}

