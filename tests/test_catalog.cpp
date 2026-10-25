#include "TestSupport.h"

void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    assert(c.size() == 3);
    assert(c.contains("B1"));
    assert(c.find("nope") == nullptr);
    assertThrows<NotFoundError>([&]() {
        (void)(c.get("nope"));
    });
    assertThrows<DuplicateIdError>([&]() {
        (void)(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1)));
    });

    assert(c.searchTitle("clean").size() == 2);
    assert(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    assert(c.where([](const Resource& r) { return r.isDigital(); }).size() == 1);

    assert(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    assert(c.holdings("B1") == 3);
    assertThrows<std::invalid_argument>([&]() {
        (void)(c.addHoldings("B1", -5));
    });

    c.remove("R1");
    assert(c.size() == 2);
    assertThrows<NotFoundError>([&]() {
        (void)(c.remove("R1"));
    });
}


void testCatalogSearches() {
    Catalog c;

    c.emplace<Book>(
        "SB1", "Clean Code",
        std::vector<std::string>{"Robert C. Martin"},
        "978-0132350884",
        "Prentice Hall", 2008,
        Money::of(100));

    c.emplace<Book>(
        "SB2", "Modern C++ Design",
        std::vector<std::string>{"Andrei Alexandrescu"},
        "978-0201704310",
        "Addison-Wesley", 2001,
        Money::of(100));

    c.emplace<EBook>(
        "SE1", "Effective Modern C++",
        std::vector<std::string>{"Scott Meyers"},
        "978-1491903995",
        EBookFormat::EPUB,
        true,
        "O'Reilly Media",
        2014,
        Money::of(100),
        "https://example.com/effective-modern-cpp",
        LicenseModel::AnnualSubscription,
        Money::of(0));

    c.emplace<EBook>(
        "SE2", "Refactoring EBook",
        std::vector<std::string>{"Martin Fowler"},
        "978-0134757599",
        EBookFormat::PDF,
        false,
        "Addison-Wesley",
        2018,
        Money::of(100),
        "https://example.com/refactoring",
        LicenseModel::AnnualSubscription,
        Money::of(0));

    c.emplace<Journal>(
        "SJ1", "Journal of Software Engineering",
        "1111-2222",
        12,
        "Tech Press",
        2020,
        Money::of(100));

    c.emplace<Journal>(
        "SJ2", "Journal of Modern Computing",
        "3333-4444",
        6,
        "ACM",
        2026,
        Money::of(100));

    c.emplace<ElectronicResource>(
        "SR1", "Research Database",
        "Research Publisher",
        2022,
        Money::of(10),
        "https://example.com/database");

    // -------------------------------------------------
    // Author search: Book
    // -------------------------------------------------

    auto robert =
        c.searchAuthor("Robert C. Martin");

    assert(robert.size() == 1);
    assert(robert[0]->id() == "SB1");

    // Case-insensitive.
    auto robertLower =
        c.searchAuthor("robert c. martin");

    assert(robertLower.size() == 1);
    assert(robertLower[0]->id() == "SB1");

    // Partial author-name matching.
    auto martin =
        c.searchAuthor("martin");

    assert(martin.size() == 2);

    assert(martin[0]->id() == "SB1");
    assert(martin[1]->id() == "SE2");

    // -------------------------------------------------
    // Author search: EBook
    // -------------------------------------------------

    auto meyers =
        c.searchAuthor("MEYERS");

    assert(meyers.size() == 1);
    assert(meyers[0]->id() == "SE1");

    auto missingAuthor =
        c.searchAuthor("No Such Author");

    assert(missingAuthor.empty());

    // -------------------------------------------------
    // ISBN search
    // -------------------------------------------------

    auto bookISBN =
        c.searchISBN("978-0132350884");

    assert(bookISBN.size() == 1);
    assert(bookISBN[0]->id() == "SB1");

    auto ebookISBN =
        c.searchISBN("978-1491903995");

    assert(ebookISBN.size() == 1);
    assert(ebookISBN[0]->id() == "SE1");

    assert(
        c.searchISBN("does-not-exist").empty());

    // Journals must not appear in ISBN search.
    assert(
        c.searchISBN("1111-2222").empty());

    // -------------------------------------------------
    // ISSN search
    // -------------------------------------------------

    auto journalISSN =
        c.searchISSN("1111-2222");

    assert(journalISSN.size() == 1);
    assert(journalISSN[0]->id() == "SJ1");

    assert(
        c.searchISSN("3333-4444").size() == 1);

    assert(
        c.searchISSN("does-not-exist").empty());

    // Books must not appear in ISSN search.
    assert(
        c.searchISSN("978-0132350884").empty());

    // -------------------------------------------------
    // Combined ISBN / ISSN identifier search
    // -------------------------------------------------

    auto idBook =
        c.searchIdentifier("978-0201704310");

    assert(idBook.size() == 1);
    assert(idBook[0]->id() == "SB2");

    auto idEBook =
        c.searchIdentifier("978-0134757599");

    assert(idEBook.size() == 1);
    assert(idEBook[0]->id() == "SE2");

    auto idJournal =
        c.searchIdentifier("3333-4444");

    assert(idJournal.size() == 1);
    assert(idJournal[0]->id() == "SJ2");

    assert(
        c.searchIdentifier("unknown-id").empty());

    // -------------------------------------------------
    // Year-range search
    // -------------------------------------------------

    auto modern =
        c.searchYearRange(2014, 2022);

    assert(modern.size() == 4);

    // Results remain in catalog id order.
    assert(modern[0]->id() == "SE1");
    assert(modern[1]->id() == "SE2");
    assert(modern[2]->id() == "SJ1");
    assert(modern[3]->id() == "SR1");

    // Inclusive lower boundary.
    auto year2014 =
        c.searchYearRange(2014, 2014);

    assert(year2014.size() == 1);
    assert(year2014[0]->id() == "SE1");

    // Inclusive upper boundary.
    auto upTo2020 =
        c.searchYearRange(2008, 2020);

    assert(upTo2020.size() == 4);

    // Future/current range.
    auto recent =
        c.searchYearRange(2022, 2026);

    assert(recent.size() == 2);
    assert(recent[0]->id() == "SJ2");
    assert(recent[1]->id() == "SR1");

    // Range containing nothing.
    assert(
        c.searchYearRange(1990, 1999).empty());

    // Invalid reversed range.
    assertThrows<std::invalid_argument>([&]() {
        (void)(c.searchYearRange(2026, 2000));
    });
}


void testLending() {
    Catalog c;

    c.emplace<Book>(
        "LB1", "Lending Book",
        std::vector<std::string>{"Author"},
        "lend-isbn",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<ElectronicResource>(
        "LR1", "Lending Database",
        "Publisher",
        2026,
        Money::of(10),
        "https://lend.example/database");

    c.emplace<EBook>(
        "LE1", "Lending EBook",
        std::vector<std::string>{"EBook Author"},
        "lend-ebook-isbn",
        EBookFormat::EPUB,
        true,
        "Publisher",
        2026,
        Money::of(20),
        "https://lend.example/ebook",
        LicenseModel::AnnualSubscription,
        Money::of(0));

    // -------------------------------------------------
    // Initial holdings
    // -------------------------------------------------

    c.addHoldings("LB1", 3);
    c.addHoldings("LR1", 2);
    c.addHoldings("LE1", 1);

    assert(c.holdings("LB1") == 3);
    assert(c.lent("LB1") == 0);
    assert(c.available("LB1") == 3);

    assert(c.holdings("LR1") == 2);
    assert(c.lent("LR1") == 0);
    assert(c.available("LR1") == 2);

    // -------------------------------------------------
    // Print lending
    // -------------------------------------------------

    c.lend("LB1");

    assert(c.lent("LB1") == 1);
    assert(c.available("LB1") == 2);

    c.lend("LB1", 2);

    assert(c.lent("LB1") == 3);
    assert(c.available("LB1") == 0);

    // Cannot lend more copies than are available.
    assertThrows<std::invalid_argument>([&]() {
        (void)(c.lend("LB1", 1));
    });

    assert(c.lent("LB1") == 3);
    assert(c.available("LB1") == 0);

    // Return one copy.
    c.returnLoan("LB1");

    assert(c.lent("LB1") == 2);
    assert(c.available("LB1") == 1);

    c.returnLoan("LB1", 2);

    assert(c.lent("LB1") == 0);
    assert(c.available("LB1") == 3);

    // Cannot return more than are lent.
    assertThrows<std::invalid_argument>([&]() {
        (void)(c.returnLoan("LB1", 1));
    });

    // -------------------------------------------------
    // Electronic concurrent sessions
    // -------------------------------------------------

    c.lend("LR1", 2);

    assert(c.lent("LR1") == 2);
    assert(c.available("LR1") == 0);

    // Both licensed seats are already in use.
    assertThrows<std::invalid_argument>([&]() {
        (void)(c.lend("LR1"));
    });

    c.returnLoan("LR1");

    assert(c.lent("LR1") == 1);
    assert(c.available("LR1") == 1);

    c.lend("LR1");

    assert(c.lent("LR1") == 2);
    assert(c.available("LR1") == 0);

    c.returnLoan("LR1", 2);

    assert(c.lent("LR1") == 0);
    assert(c.available("LR1") == 2);

    // -------------------------------------------------
    // EBook also behaves as a digital session resource
    // because it inherits ElectronicResource.
    // -------------------------------------------------

    assert(c.get("LE1").isDigital());

    c.lend("LE1");

    assert(c.lent("LE1") == 1);
    assert(c.available("LE1") == 0);

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.lend("LE1"));
    });

    c.returnLoan("LE1");

    assert(c.lent("LE1") == 0);
    assert(c.available("LE1") == 1);

    // -------------------------------------------------
    // Invalid operations
    // -------------------------------------------------

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.lend("LB1", 0));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.lend("LB1", -1));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.returnLoan("LB1", 0));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.returnLoan("LB1", -1));
    });

    assertThrows<NotFoundError>([&]() {
        (void)(c.lend("NOPE"));
    });

    assertThrows<NotFoundError>([&]() {
        (void)(c.returnLoan("NOPE"));
    });

    assertThrows<NotFoundError>([&]() {
        (void)(c.lent("NOPE"));
    });

    assertThrows<NotFoundError>([&]() {
        (void)(c.available("NOPE"));
    });

    // -------------------------------------------------
    // Holdings cannot be reduced below units currently
    // lent / active.
    // -------------------------------------------------

    c.lend("LB1", 2);

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.addHoldings("LB1", -2));
    });

    assert(c.holdings("LB1") == 3);
    assert(c.lent("LB1") == 2);
    assert(c.available("LB1") == 1);

    c.returnLoan("LB1", 2);

    // Once returned, holdings may be reduced normally.
    c.addHoldings("LB1", -2);

    assert(c.holdings("LB1") == 1);
    assert(c.available("LB1") == 1);

    // -------------------------------------------------
    // Acquisition integration:
    // purchased copies become available for lending.
    // -------------------------------------------------

    Catalog purchaseCatalog;

    purchaseCatalog.emplace<Book>(
        "PB1", "Purchased Lending Book",
        std::vector<std::string>{"Author"},
        "purchase-lend-isbn",
        "Publisher",
        2026,
        Money::of(100));

    purchaseCatalog.emplace<ElectronicResource>(
        "PR1", "Purchased Digital Resource",
        "Publisher",
        2026,
        Money::of(10),
        "https://purchase-lend.example");

    Budget purchaseBudget(Money::of(5000));

    purchaseBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(2000), 5});

    purchaseBudget.setQuota(
        ResourceCategory::ElectronicResource,
        {10, Money::of(2000), 5});

    AcquisitionManager acq(
        purchaseCatalog,
        purchaseBudget);

    acq.purchase("PB1", 3);
    acq.purchase("PR1", 2);

    assert(purchaseCatalog.holdings("PB1") == 3);
    assert(purchaseCatalog.available("PB1") == 3);

    assert(purchaseCatalog.holdings("PR1") == 2);
    assert(purchaseCatalog.available("PR1") == 2);

    purchaseCatalog.lend("PB1", 2);
    purchaseCatalog.lend("PR1", 2);

    assert(purchaseCatalog.available("PB1") == 1);
    assert(purchaseCatalog.available("PR1") == 0);

    // -------------------------------------------------
    // Q8 + Q14 interaction:
    // cancellation must not remove currently lent copies.
    // -------------------------------------------------

    Catalog cancelCatalog;

    cancelCatalog.emplace<Book>(
        "CL1", "Cancellation Lending Book",
        std::vector<std::string>{"Author"},
        "cancel-lending-isbn",
        "Publisher",
        2026,
        Money::of(100));

    Budget cancelBudget(Money::of(1000));

    cancelBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000), 5});

    AcquisitionManager cancelAcq(
        cancelCatalog,
        cancelBudget);

    const int orderNo =
        cancelAcq.purchase("CL1", 2).orderNo;

    assert(cancelCatalog.holdings("CL1") == 2);
    assert(cancelBudget.spent() == Money::of(200));

    cancelCatalog.lend("CL1", 1);

    // One copy is currently lent, so the purchase of
    // two copies cannot yet be cancelled.
    assertThrows<std::invalid_argument>([&]() {
        (void)(cancelAcq.cancel(orderNo));
    });

    // Failed cancellation must not have refunded anything.
    assert(cancelBudget.spent() == Money::of(200));
    assert(cancelCatalog.holdings("CL1") == 2);
    assert(cancelCatalog.lent("CL1") == 1);

    cancelCatalog.returnLoan("CL1");

    const PurchaseRecord& cancelled =
        cancelAcq.cancel(orderNo);

    assert(cancelled.type == RecordType::Cancellation);
    assert(cancelCatalog.holdings("CL1") == 0);
    assert(cancelCatalog.lent("CL1") == 0);
    assert(cancelCatalog.available("CL1") == 0);
    assert(cancelBudget.spent() == Money::of(0));
}

