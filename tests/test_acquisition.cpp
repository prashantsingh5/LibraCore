#include "TestSupport.h"

void testAcquisition() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    assert(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    assert(acq.canPurchase("B1", 3, &why) && why.empty());
    assert(!acq.canPurchase("B1", 4, &why) && !why.empty());
    assert(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    assert(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    assert(c.holdings("B1") == 2);

    assertThrows<QuotaExceededError>([&]() {
        (void)(acq.purchase("B1", 2));
    });
    assertThrows<NotFoundError>([&]() {
        (void)(acq.purchase("nope", 1));
    });
    assert(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    assert(res.size() == 5);
    assert(res[0].approved && res[0].cost == Money::of(150));
    assert(!res[1].approved);  // 1050 > remaining 150
    assert(res[2].approved);
    assert(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    assert(!res[4].approved);
    assert(acq.totalSpent() == Money::of(450));
    assert(b.spent() == acq.totalSpent());
    assert(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    assert(acq.history().size() == 6);
}


void testTaxes() {
    Catalog c;

    c.emplace<Book>("TB1", "Taxed Print Book",
                    std::vector<std::string>{"Author"},
                    "tax-isbn", "Publisher", 2026,
                    Money::of(100));

    c.emplace<ElectronicResource>(
        "TR1", "Taxed Electronic Resource",
        "Publisher", 2026,
        Money::of(10),
        "https://tax.example/resource",
        LicenseModel::AnnualSubscription,
        Money::of(50));

    c.emplace<EBook>(
        "TE1", "Taxed EBook",
        std::vector<std::string>{"Author"},
        "tax-ebook-isbn",
        EBookFormat::PDF,
        false,
        "Publisher", 2026,
        Money::of(100),
        "https://tax.example/ebook",
        LicenseModel::AnnualSubscription);

    Budget b(Money::of(2000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(1000)});
    b.setQuota(ResourceCategory::ElectronicResource, {20, Money::of(1000)});
    b.setQuota(ResourceCategory::EBook, {10, Money::of(1000)});

    // Q6: print tax = 10%, electronic tax = 20%.
    AcquisitionManager acq(c, b, TaxRates{10, 20});

    assert(acq.taxRates().printPercent == 10);
    assert(acq.taxRates().electronicPercent == 20);

    // Print: 100 pre-tax -> 110 post-tax.
    assert(acq.quote("TB1", 1) == Money::of(110));

    // Electronic: platform fee 50 + 5 * 10 = 100 pre-tax.
    // 20% electronic tax -> 120 post-tax.
    assert(acq.quote("TR1", 5) == Money::of(120));

    // EBook has its own ResourceCategory, but it is still electronic.
    assert(acq.quote("TE1", 1) == Money::of(120));

    const PurchaseRecord& printRec = acq.purchase("TB1", 2);

    assert(printRec.approved);
    assert(printRec.preTaxCost == Money::of(200));
    assert(printRec.cost == Money::of(220));

    const PurchaseRecord& electronicRec = acq.purchase("TR1", 5);

    assert(electronicRec.approved);
    assert(electronicRec.preTaxCost == Money::of(100));
    assert(electronicRec.cost == Money::of(120));

    assert(acq.totalPreTax() == Money::of(300));
    assert(acq.totalSpent() == Money::of(340));
    assert(b.spent() == Money::of(340));

    std::ostringstream report;
    acq.printReport(report);

    assert(report.str().find("pre-tax") != std::string::npos);
    assert(report.str().find("post-tax") != std::string::npos);
    assert(report.str().find("Total pre-tax: 300.00") != std::string::npos);
    assert(report.str().find("Total post-tax: 340.00") != std::string::npos);

    // Tax rates are configurable after construction.
    acq.setTaxRates(TaxRates{5, 15});
    assert(acq.taxRates().printPercent == 5);
    assert(acq.taxRates().electronicPercent == 15);
    assert(acq.quote("TB1", 1) == Money::of(105));

    Catalog quotaCatalog;

    quotaCatalog.emplace<Book>(
        "QB1", "Quota Tax Book",
        std::vector<std::string>{"Author"},
        "quota-isbn", "Publisher", 2026,
        Money::of(100));

    Budget quotaBudget(Money::of(1000));

    quotaBudget.setQuota(ResourceCategory::Book,
                         {10, Money::of(105)});

    AcquisitionManager quotaAcq(
        quotaCatalog, quotaBudget, TaxRates{10, 20});

    std::string why;
    assert(!quotaAcq.canPurchase("QB1", 1, &why));
    assert(!why.empty());

    assertThrows<QuotaExceededError>([&]() {
        (void)(quotaAcq.purchase("QB1", 1));
    });

    assert(quotaBudget.spent() == Money::of(0));
    assert(quotaCatalog.holdings("QB1") == 0);

    assertThrows<std::invalid_argument>([&]() {
        (void)(acq.setTaxRates(TaxRates{-1, 10}));
    });
}


void testCancellation() {
    Catalog c;

    c.emplace<Book>(
        "B1", "Cancellation Book One",
        std::vector<std::string>{"Author"},
        "cancel-isbn-1", "Publisher", 2026,
        Money::of(100));

    c.emplace<Book>(
        "B2", "Cancellation Book Two",
        std::vector<std::string>{"Author"},
        "cancel-isbn-2", "Publisher", 2026,
        Money::of(100));

    c.emplace<Book>(
        "B3", "Cancellation Book Three",
        std::vector<std::string>{"Author"},
        "cancel-isbn-3", "Publisher", 2026,
        Money::of(100));

    Budget b(Money::of(5000));

    // Maximum 20 copies, 5000 spend and 2 different titles.
    b.setQuota(ResourceCategory::Book,
               {20, Money::of(5000), 2});

    // 10% print tax lets us verify that cancellation refunds
    // the exact post-tax amount originally charged.
    AcquisitionManager acq(c, b, TaxRates{10, 20});

    // First purchase: 2 * 100 = 200 pre-tax, 220 post-tax.
    const int firstOrderNo = acq.purchase("B1", 2).orderNo;

    assert(c.holdings("B1") == 2);
    assert(b.spent() == Money::of(220));
    assert(b.usageFor(ResourceCategory::Book).units == 2);
    assert(b.titlesUsed(ResourceCategory::Book) == 1);

    // Buy the same title again.
    const int secondOrderNo = acq.purchase("B1", 1).orderNo;

    assert(c.holdings("B1") == 3);
    assert(b.spent() == Money::of(330));
    assert(b.titlesUsed(ResourceCategory::Book) == 1);

    // Buy a second different title.
    const int thirdOrderNo = acq.purchase("B2", 1).orderNo;

    assert(thirdOrderNo == 3);
    assert(c.holdings("B2") == 1);
    assert(b.spent() == Money::of(440));
    assert(b.usageFor(ResourceCategory::Book).units == 4);
    assert(b.titlesUsed(ResourceCategory::Book) == 2);
    assert(*b.titlesRemaining(ResourceCategory::Book) == 0);

    // Third different title is currently blocked.
    std::string why;
    assert(!acq.canPurchase("B3", 1, &why));
    assert(why.find("title quota exceeded") != std::string::npos);

    // Record one rejected request so that cancelling a rejected
    // order can also be tested.
    auto rejected = acq.processBatch({{"B3", 1}});

    assert(rejected.size() == 1);
    assert(!rejected[0].approved);

    const int rejectedOrderNo = rejected[0].orderNo;

    // Cancel the first B1 purchase.
    const PurchaseRecord& cancellation1 =
        acq.cancel(firstOrderNo);

    assert(cancellation1.type == RecordType::Cancellation);
    assert(cancellation1.relatedOrderNo == firstOrderNo);
    assert(cancellation1.approved);
    assert(cancellation1.resourceId == "B1");
    assert(cancellation1.quantity == 2);
    assert(cancellation1.preTaxCost == Money::of(200));
    assert(cancellation1.cost == Money::of(220));

    // Exact budget and quota usage must be refunded.
    assert(b.spent() == Money::of(220));
    assert(b.usageFor(ResourceCategory::Book).units == 2);

    // Holdings drop from 3 to 1.
    assert(c.holdings("B1") == 1);

    // B1 still has another active purchase, so it must still count
    // as a distinct title.
    assert(b.titlesUsed(ResourceCategory::Book) == 2);
    assert(*b.titlesRemaining(ResourceCategory::Book) == 0);

    // Therefore B3 is still blocked.
    assert(!acq.canPurchase("B3", 1));

    // Net acquisition totals must also reflect the cancellation.
    assert(acq.totalPreTax() == Money::of(200));
    assert(acq.totalSpent() == Money::of(220));
    assert(acq.totalSpent() == b.spent());

    // The same purchase cannot be cancelled twice.
    assertThrows<std::invalid_argument>([&]() {
        (void)(acq.cancel(firstOrderNo));
    });

    // A rejected request cannot be cancelled.
    assertThrows<std::invalid_argument>([&]() {
        (void)(acq.cancel(rejectedOrderNo));
    });

    // A nonexistent order cannot be cancelled.
    assertThrows<std::invalid_argument>([&]() {
        (void)(acq.cancel(9999));
    });

    // Cancel the remaining active B1 purchase.
    const PurchaseRecord& cancellation2 =
        acq.cancel(secondOrderNo);

    assert(cancellation2.type == RecordType::Cancellation);
    assert(cancellation2.relatedOrderNo == secondOrderNo);

    assert(c.holdings("B1") == 0);

    // Now B1 has no active purchased units, so its title slot is freed.
    assert(b.titlesUsed(ResourceCategory::Book) == 1);
    assert(*b.titlesRemaining(ResourceCategory::Book) == 1);

    assert(b.spent() == Money::of(110));
    assert(b.usageFor(ResourceCategory::Book).units == 1);

    assert(acq.totalPreTax() == Money::of(100));
    assert(acq.totalSpent() == Money::of(110));

    // With the title slot restored, B3 can now be purchased.
    assert(acq.canPurchase("B3", 1));

    const int newOrderNo = acq.purchase("B3", 1).orderNo;

    assert(newOrderNo > secondOrderNo);
    assert(c.holdings("B3") == 1);
    assert(b.titlesUsed(ResourceCategory::Book) == 2);

    assert(b.spent() == Money::of(220));
    assert(acq.totalSpent() == Money::of(220));

    // Original purchase records must remain in history.
    assert(acq.history()[0].orderNo == firstOrderNo);
    assert(acq.history()[0].type == RecordType::Purchase);
    assert(acq.history()[0].approved);

    // Cancellation records are separate history entries.
    bool foundFirstCancellation = false;
    bool foundSecondCancellation = false;

    for (const auto& rec : acq.history()) {
        if (rec.type == RecordType::Cancellation &&
            rec.relatedOrderNo == firstOrderNo) {
            foundFirstCancellation = true;
        }

        if (rec.type == RecordType::Cancellation &&
            rec.relatedOrderNo == secondOrderNo) {
            foundSecondCancellation = true;
        }
    }

    assert(foundFirstCancellation);
    assert(foundSecondCancellation);

    // Report should clearly display cancellation records.
    std::ostringstream report;
    acq.printReport(report);

    assert(report.str().find("CANCELLED") != std::string::npos);
    assert(report.str().find("cancels order #1") != std::string::npos);
}


void testDepartments() {
    Catalog c;

    c.emplace<Book>(
        "B1", "Department Book",
        std::vector<std::string>{"Author"},
        "dept-isbn-1", "Publisher", 2026,
        Money::of(100));

    c.emplace<Book>(
        "B2", "Second Department Book",
        std::vector<std::string>{"Author"},
        "dept-isbn-2", "Publisher", 2026,
        Money::of(100));

    Budget defaultBudget(Money::of(5000));

    Budget csBudget(Money::of(1000));
    csBudget.setQuota(ResourceCategory::Book,
                      {3, Money::of(500), 2});

    Budget physicsBudget(Money::of(2000));
    physicsBudget.setQuota(ResourceCategory::Book,
                           {10, Money::of(1500), 3});

    AcquisitionManager acq(c, defaultBudget);

    acq.addDepartment("Computer Science", csBudget);
    acq.addDepartment("Physics", physicsBudget);

    assert(acq.hasDepartment("Computer Science"));
    assert(acq.hasDepartment("Physics"));
    assert(!acq.hasDepartment("Chemistry"));

    // Same resource can be charged independently to different departments.
    const auto& csRec =
        acq.purchase("B1", 2, "Computer Science");

    assert(csRec.approved);
    assert(csRec.department == "Computer Science");
    assert(csBudget.spent() == Money::of(200));
    assert(csBudget.usageFor(ResourceCategory::Book).units == 2);

    const auto& phyRec =
        acq.purchase("B1", 3, "Physics");

    assert(phyRec.approved);
    assert(phyRec.department == "Physics");
    assert(physicsBudget.spent() == Money::of(300));
    assert(physicsBudget.usageFor(ResourceCategory::Book).units == 3);

    // Holdings are global to the catalog.
    assert(c.holdings("B1") == 5);

    // The CS unit quota has only 1 unit left.
    std::string why;
    assert(!acq.canPurchase("B1", 2, "Computer Science", &why));
    assert(!why.empty());

    // Physics still has its own independent quota.
    assert(acq.canPurchase("B1", 2, "Physics"));

    // Different title charged to CS.
    const auto& csSecond =
        acq.purchase("B2", 1, "Computer Science");

    assert(csSecond.department == "Computer Science");
    assert(csBudget.spent() == Money::of(300));
    assert(csBudget.usageFor(ResourceCategory::Book).units == 3);

    // CS is now at its unit limit.
    assert(!acq.canPurchase("B1", 1, "Computer Science"));

    // Physics remains unaffected.
    assert(acq.canPurchase("B2", 1, "Physics"));

    // Unknown department should not be accepted.
    assert(!acq.canPurchase("B1", 1, "Chemistry", &why));
    assert(why.find("department not found") != std::string::npos);

    assertThrows<std::invalid_argument>([&]() {
        (void)(acq.purchase("B1", 1, "Chemistry"));
    });

    // Batch requests must use the named department.
    auto batch = acq.processBatch({
        {"B2", 1, "Physics"},
        {"B1", 1, "Computer Science"},
        {"B1", 1, "Chemistry"}
    });

    assert(batch.size() == 3);

    assert(batch[0].approved);
    assert(batch[0].department == "Physics");

    assert(!batch[1].approved);
    assert(batch[1].department == "Computer Science");

    assert(!batch[2].approved);
    assert(batch[2].department == "Chemistry");
    assert(batch[2].reason.find("department not found") != std::string::npos);

    assert(physicsBudget.spent() == Money::of(400));

    // Cancellation must refund the department that originally paid.
    const int physicsOrderNo = batch[0].orderNo;

    const auto& cancellation =
        acq.cancel(physicsOrderNo);

    assert(cancellation.type == RecordType::Cancellation);
    assert(cancellation.department == "Physics");
    assert(cancellation.relatedOrderNo == physicsOrderNo);

    assert(physicsBudget.spent() == Money::of(300));

    // CS budget must be unchanged by a Physics cancellation.
    assert(csBudget.spent() == Money::of(300));

    // Department should appear in reports.
    std::ostringstream report;
    acq.printReport(report);

    assert(report.str().find("[department: Computer Science]") !=
          std::string::npos);

    assert(report.str().find("[department: Physics]") !=
          std::string::npos);

    // Duplicate and empty department registration are invalid.
    assertThrows<std::invalid_argument>([&]() {
        (void)(acq.addDepartment("Computer Science", csBudget));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(acq.addDepartment("", csBudget));
    });
}


void testAtomicBatch() {
    Catalog c;

    c.emplace<Book>(
        "B1", "Atomic Book One",
        std::vector<std::string>{"Author"},
        "atomic-isbn-1", "Publisher", 2026,
        Money::of(100));

    c.emplace<Book>(
        "B2", "Atomic Book Two",
        std::vector<std::string>{"Author"},
        "atomic-isbn-2", "Publisher", 2026,
        Money::of(100));

    c.emplace<Book>(
        "B3", "Atomic Book Three",
        std::vector<std::string>{"Author"},
        "atomic-isbn-3", "Publisher", 2026,
        Money::of(100));

    Budget b(Money::of(5000));

    b.setQuota(
        ResourceCategory::Book,
        {5, Money::of(1000), 2});

    AcquisitionManager acq(c, b);

    // -------------------------------------------------
    // Successful atomic batch
    // -------------------------------------------------

    auto success = acq.processBatch(
        {
            {"B1", 2},
            {"B2", 1}
        },
        true);

    assert(success.size() == 2);
    assert(success[0].approved);
    assert(success[1].approved);

    assert(c.holdings("B1") == 2);
    assert(c.holdings("B2") == 1);

    assert(b.spent() == Money::of(300));
    assert(b.usageFor(ResourceCategory::Book).units == 3);
    assert(b.titlesUsed(ResourceCategory::Book) == 2);

    // -------------------------------------------------
    // Atomic batch that fails because of cumulative
    // unit quota usage.
    //
    // 2 copies fit individually when checked against
    // the current state, but two requests together do not.
    // -------------------------------------------------

    const std::size_t historyBefore =
        acq.history().size();

    const Money spentBefore = b.spent();

    const int unitsBefore =
        b.usageFor(ResourceCategory::Book).units;

    const int b1HoldingsBefore =
        c.holdings("B1");

    const int b2HoldingsBefore =
        c.holdings("B2");

    auto failed = acq.processBatch(
        {
            {"B1", 1},
            {"B2", 2}
        },
        true);

    assert(failed.size() == 2);

    assert(!failed[0].approved);
    assert(!failed[1].approved);

    // Nothing real must have changed.
    assert(b.spent() == spentBefore);
    assert(b.usageFor(ResourceCategory::Book).units == unitsBefore);

    assert(c.holdings("B1") == b1HoldingsBefore);
    assert(c.holdings("B2") == b2HoldingsBefore);

    // Rejected atomic requests are still recorded.
    assert(acq.history().size() == historyBefore + 2);

    // At least one record should identify the actual
    // quota failure.
    bool foundQuotaFailure = false;
    bool foundAtomicRejection = false;

    for (const auto& rec : failed) {
        if (rec.reason.find("quota exceeded") != std::string::npos) {
            foundQuotaFailure = true;
        }

        if (rec.reason.find("batch rejected") != std::string::npos) {
            foundAtomicRejection = true;
        }
    }

    assert(foundQuotaFailure);
    assert(foundAtomicRejection);

    // -------------------------------------------------
    // Atomic batch fails because of unknown resource.
    // The valid request must also not be purchased.
    // -------------------------------------------------

    const int beforeUnknownB1 =
        c.holdings("B1");

    const Money beforeUnknownSpent =
        b.spent();

    auto unknown = acq.processBatch(
        {
            {"B1", 1},
            {"NOPE", 1}
        },
        true);

    assert(unknown.size() == 2);
    assert(!unknown[0].approved);
    assert(!unknown[1].approved);

    assert(c.holdings("B1") == beforeUnknownB1);
    assert(b.spent() == beforeUnknownSpent);

    assert(
        unknown[1].reason.find("resource not found") !=
        std::string::npos);

    // -------------------------------------------------
    // Verify original non-atomic behavior is unchanged.
    // First request may succeed even if a later one fails.
    // -------------------------------------------------

    Catalog normalCatalog;

    normalCatalog.emplace<Book>(
        "NB1", "Normal Batch Book",
        std::vector<std::string>{"Author"},
        "normal-isbn", "Publisher", 2026,
        Money::of(100));

    Budget normalBudget(Money::of(1000));

    normalBudget.setQuota(
        ResourceCategory::Book,
        {2, Money::of(1000), 2});

    AcquisitionManager normalAcq(
        normalCatalog, normalBudget);

    auto normal = normalAcq.processBatch({
        {"NB1", 1},
        {"NB1", 2}
    });

    assert(normal.size() == 2);
    assert(normal[0].approved);
    assert(!normal[1].approved);

    assert(normalCatalog.holdings("NB1") == 1);
    assert(normalBudget.spent() == Money::of(100));

    // -------------------------------------------------
    // Department budgets must also be simulated
    // independently in atomic mode.
    // -------------------------------------------------

    Catalog deptCatalog;

    deptCatalog.emplace<Book>(
        "DB1", "Department Atomic Book",
        std::vector<std::string>{"Author"},
        "dept-atomic-isbn", "Publisher", 2026,
        Money::of(100));

    Budget defaultBudget(Money::of(5000));

    Budget csBudget(Money::of(1000));
    csBudget.setQuota(
        ResourceCategory::Book,
        {2, Money::of(1000), 2});

    Budget physicsBudget(Money::of(1000));
    physicsBudget.setQuota(
        ResourceCategory::Book,
        {5, Money::of(1000), 2});

    AcquisitionManager deptAcq(
        deptCatalog, defaultBudget);

    deptAcq.addDepartment(
        "Computer Science", csBudget);

    deptAcq.addDepartment(
        "Physics", physicsBudget);

    auto deptFail = deptAcq.processBatch(
        {
            {"DB1", 2, "Computer Science"},
            {"DB1", 1, "Computer Science"},
            {"DB1", 2, "Physics"}
        },
        true);

    assert(deptFail.size() == 3);

    assert(!deptFail[0].approved);
    assert(!deptFail[1].approved);
    assert(!deptFail[2].approved);

    // CS second request makes CS cumulative usage exceed 2,
    // so the entire batch must be rejected, including Physics.
    assert(csBudget.spent() == Money::of(0));
    assert(physicsBudget.spent() == Money::of(0));
    assert(deptCatalog.holdings("DB1") == 0);

    // -------------------------------------------------
    // Title quota must also be cumulative in simulation.
    // -------------------------------------------------

    Catalog titleCatalog;

    titleCatalog.emplace<Book>(
        "T1", "Atomic Title One",
        std::vector<std::string>{"Author"},
        "t1", "Publisher", 2026,
        Money::of(100));

    titleCatalog.emplace<Book>(
        "T2", "Atomic Title Two",
        std::vector<std::string>{"Author"},
        "t2", "Publisher", 2026,
        Money::of(100));

    Budget titleBudget(Money::of(1000));

    titleBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000), 1});

    AcquisitionManager titleAcq(
        titleCatalog, titleBudget);

    auto titleFail = titleAcq.processBatch(
        {
            {"T1", 1},
            {"T2", 1}
        },
        true);

    assert(titleFail.size() == 2);
    assert(!titleFail[0].approved);
    assert(!titleFail[1].approved);

    assert(titleBudget.spent() == Money::of(0));
    assert(titleBudget.titlesUsed(ResourceCategory::Book) == 0);

    assert(titleCatalog.holdings("T1") == 0);
    assert(titleCatalog.holdings("T2") == 0);
}


void testVendors() {
    Catalog c;

    auto& book = c.emplace<Book>(
        "VB1", "Vendor Book",
        std::vector<std::string>{"Author"},
        "vendor-isbn", "Publisher", 2026,
        Money::of(120));

    // Multiple vendors for the same title.
    book.addVendor("Vendor A", Money::of(100));
    book.addVendor("Vendor B", Money::of(80));
    book.addVendor("Vendor C", Money::of(90));

    auto& hardcover = c.emplace<Book>(
        "VB2", "Vendor Hardcover",
        std::vector<std::string>{"Author"},
        "vendor-hardcover-isbn", "Publisher", 2026,
        Money::of(200),
        1,
        Binding::Hardcover);

    hardcover.addVendor("Hardcover Vendor A", Money::of(100));
    hardcover.addVendor("Hardcover Vendor B", Money::of(90));

    auto& electronic = c.emplace<ElectronicResource>(
        "VR1", "Vendor Database",
        "Publisher", 2026,
        Money::of(20),
        "https://vendor.example/db",
        LicenseModel::AnnualSubscription,
        Money::of(50));

    electronic.addVendor("Digital Vendor A", Money::of(15));
    electronic.addVendor("Digital Vendor B", Money::of(10));

    auto& journal = c.emplace<Journal>(
        "VJ1", "Vendor Journal",
        "1111-2222", 12,
        "Publisher", 2026,
        Money::of(150),
        2);

    journal.addVendor("Journal Vendor A", Money::of(100));
    journal.addVendor("Journal Vendor B", Money::of(90));

    Budget b(Money::of(10000));

    b.setQuota(ResourceCategory::Book,
               {50, Money::of(5000), 10});

    b.setQuota(ResourceCategory::ElectronicResource,
               {100, Money::of(5000), 10});

    b.setQuota(ResourceCategory::Journal,
               {20, Money::of(5000), 10});

    AcquisitionManager acq(
        c, b, TaxRates{10, 20});

    // Cheapest vendor must be selected.
    assert(book.cheapestVendor() != nullptr);
    assert(book.cheapestVendor()->vendor == "Vendor B");
    assert(book.cheapestVendor()->price == Money::of(80));

    // Listed unit price remains unchanged.
    assert(book.unitPrice() == Money::of(120));

    // Vendor Book:
    // 2 * 80 = 160 pre-tax, +10% = 176.
    assert(acq.quote("VB1", 2) == Money::of(176));

    const PurchaseRecord& bookRec =
        acq.purchase("VB1", 2);

    assert(bookRec.approved);
    assert(bookRec.vendor == "Vendor B");
    assert(bookRec.preTaxCost == Money::of(160));
    assert(bookRec.cost == Money::of(176));

    // Store the order number immediately because later purchases may
    // reallocate AcquisitionManager's history vector.
    const int bookOrderNo = bookRec.orderNo;

    // Hardcover vendor pricing:
    // cheapest price = 90
    // 2 * 90 = 180
    // hardcover +20% = 216
    // print tax +10% = 237.60
    assert(acq.quote("VB2", 2) ==
          Money::fromMinor(23760));

    const PurchaseRecord& hardRec =
        acq.purchase("VB2", 2);

    assert(hardRec.vendor == "Hardcover Vendor B");
    assert(hardRec.preTaxCost == Money::of(216));
    assert(hardRec.cost == Money::fromMinor(23760));

    // Electronic resource:
    // platform fee 50 + 5 * 10 = 100 pre-tax
    // +20% tax = 120.
    assert(acq.quote("VR1", 5) == Money::of(120));

    const PurchaseRecord& electronicRec =
        acq.purchase("VR1", 5);

    assert(electronicRec.vendor == "Digital Vendor B");
    assert(electronicRec.preTaxCost == Money::of(100));
    assert(electronicRec.cost == Money::of(120));

    // Journal:
    // cheapest annual price = 90
    // 90 * 2 copies * 2 subscription years = 360
    // +10% print tax = 396.
    assert(acq.quote("VJ1", 2) == Money::of(396));

    const PurchaseRecord& journalRec =
        acq.purchase("VJ1", 2);

    assert(journalRec.vendor == "Journal Vendor B");
    assert(journalRec.preTaxCost == Money::of(360));
    assert(journalRec.cost == Money::of(396));

    // Q5 bulk pricing must still apply using the vendor price.
    //
    // VB1 vendor price = 80
    // 10 copies = 800
    // 10% bulk discount = 720
    // 10% tax = 792.
    assert(acq.quote("VB1", 10) == Money::of(792));

    // Electronic >50-seat pricing must still use vendor seat price.
    //
    // vendor = 10
    // 50 full seats = 500
    // 10 half-price seats = 50
    // platform fee = 50
    // pre-tax = 600
    // +20% = 720.
    assert(acq.quote("VR1", 60) == Money::of(720));

    // Resource without vendors still uses listed price and blank vendor.
    c.emplace<Book>(
        "NV1", "No Vendor Book",
        std::vector<std::string>{"Author"},
        "no-vendor-isbn", "Publisher", 2026,
        Money::of(100));

    const PurchaseRecord& noVendor =
        acq.purchase("NV1", 1);

    assert(noVendor.vendor.empty());
    assert(noVendor.preTaxCost == Money::of(100));
    assert(noVendor.cost == Money::of(110));

    // Invalid vendor entries.
    assertThrows<std::invalid_argument>([&]() {
        (void)(book.addVendor("", Money::of(10)));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(book.addVendor("Bad Vendor", Money::of(-1)));
    });

    // Report must show selected vendor.
    std::ostringstream report;
    acq.printReport(report);

    assert(report.str().find("[vendor: Vendor B]") !=
          std::string::npos);

    assert(report.str().find("[vendor: Digital Vendor B]") !=
          std::string::npos);

    // Cancellation should preserve the original vendor.
    const PurchaseRecord& cancelled =
        acq.cancel(bookOrderNo);

    assert(cancelled.type == RecordType::Cancellation);
    assert(cancelled.vendor == "Vendor B");
    assert(cancelled.relatedOrderNo == bookOrderNo);

    // Atomic batch must use vendor prices during simulation.
    Catalog atomicCatalog;

    auto& atomicBook =
        atomicCatalog.emplace<Book>(
            "AV1", "Atomic Vendor Book",
            std::vector<std::string>{"Author"},
            "atomic-vendor-isbn",
            "Publisher", 2026,
            Money::of(200));

    atomicBook.addVendor(
        "Cheap Vendor", Money::of(50));

    Budget atomicBudget(Money::of(100));

    atomicBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(100), 5});

    AcquisitionManager atomicAcq(
        atomicCatalog, atomicBudget);

    auto atomic = atomicAcq.processBatch(
        {
            {"AV1", 1},
            {"AV1", 1}
        },
        true);

    assert(atomic.size() == 2);
    assert(atomic[0].approved);
    assert(atomic[1].approved);

    assert(atomic[0].vendor == "Cheap Vendor");
    assert(atomic[1].vendor == "Cheap Vendor");

    assert(atomicBudget.spent() == Money::of(100));
    assert(atomicCatalog.holdings("AV1") == 2);
}

