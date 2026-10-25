#include "TestSupport.h"

void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    assert(b.category() == ResourceCategory::Book);
    assert(!b.isDigital());
    assert(b.costFor(3) == Money::of(300));
    assertThrows<std::invalid_argument>([&]() {
        (void)(b.costFor(0));
    });
    assert(joinAuthors(b.authors()) == "A, B and C");

    // Q4: Hardcover books cost 20% more than the listed unit price.
    Book paperback("PB1", "Paperback Test", {"Author"}, "isbn-pb", "P", 2026,
                Money::of(100), 1, Binding::Paperback);

    Book hardcover("HB1", "Hardcover Test", {"Author"}, "isbn-hb", "P", 2026,
                Money::of(100), 1, Binding::Hardcover);

    assert(paperback.costFor(2) == Money::of(200));
    assert(hardcover.costFor(1) == Money::of(120));
    assert(hardcover.costFor(2) == Money::of(240));
    assertThrows<std::invalid_argument>([&]() {
        (void)(hardcover.costFor(0));
    });
    Book bulkPaperback("PB2", "Bulk Paperback", {"Author"}, "isbn-bulk", "P", 2026,
                    Money::of(100), 1, Binding::Paperback);

    assert(bulkPaperback.costFor(9) == Money::of(900));
    assert(bulkPaperback.costFor(10) == Money::of(900));

    Book bulkHardcover("HB2", "Bulk Hardcover", {"Author"}, "isbn-bulk-hc", "P", 2026,
                    Money::of(100), 1, Binding::Hardcover);

    assert(bulkHardcover.costFor(10) == Money::of(1080));

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    assert(e.isDigital());
    assert(e.costFor(5) == Money::of(150));

    assert(e.category() == ResourceCategory::ElectronicResource);
    ElectronicResource bulkElectronic(
        "R2", "Bulk Digital Resource", "P", 2026,
        Money::of(100), "url",
        LicenseModel::AnnualSubscription, Money::of(500));

    assert(bulkElectronic.costFor(50) == Money::of(5500));
    assert(bulkElectronic.costFor(60) == Money::of(6000));

    // Polymorphism through a base-class reference
    const Resource& r = e;
    assert(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    assert(os.str().find("platform fee: 100.00") != std::string::npos);

    assertThrows<std::invalid_argument>([&]() {
        (void)(Book("", "T", {}, "", "", 2000, Money::of(1)));
    });
    assertThrows<std::invalid_argument>([&]() {
        (void)(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)));
    });
}


void testJournal() {
    Journal j("J1", "Journal of Computing", "1234-5678", 12,
              "Tech Press", 2026, Money::of(100));

    assert(j.category() == ResourceCategory::Journal);
    assert(std::string(categoryName(j.category())) == "Journal");
    assert(j.issn() == "1234-5678");
    assert(j.issuesPerYear() == 12);
    assert(j.subscriptionYears() == 1);
    assert(j.costFor(3) == Money::of(300));
    assertThrows<std::invalid_argument>([&]() {
        (void)(j.costFor(0));
    });

    Journal multiYear("J2", "Software Journal", "9876-5432", 6,
                      "ACM", 2026, Money::of(250), 2);

    assert(multiYear.subscriptionYears() == 2);
    assert(multiYear.costFor(3) == Money::of(1500));

    assertThrows<std::invalid_argument>([&]() {
        (void)(Journal("J3", "Invalid Journal", "1111-2222", 4,
                "Publisher", 2026, Money::of(100), 0));
    });

    std::ostringstream os;
    os << multiYear;
    assert(os.str().find("issn: 9876-5432") != std::string::npos);
    assert(os.str().find("issues per year: 6") != std::string::npos);
    assert(os.str().find("subscription years: 2") != std::string::npos);
    Journal bulkJournal("J4", "Bulk Journal", "2222-3333", 12,
                        "Publisher", 2026, Money::of(100), 2);
    assert(bulkJournal.costFor(9) == Money::of(1800));
    assert(bulkJournal.costFor(10) == Money::of(1800));
}


void testEBook() {
    EBook e("E1", "Modern C++ EBook",
            std::vector<std::string>{"Author One", "Author Two"},
            "978-1234567890",
            EBookFormat::EPUB,
            true,
            "Tech Press",
            2026,
            Money::of(100),
            "https://example.com/ebook",
            LicenseModel::AnnualSubscription,
            Money::of(500));

    assert(e.category() == ResourceCategory::EBook);
    assert(std::string(categoryName(e.category())) == "EBook");

    assert(e.authors().size() == 2);
    assert(e.authors()[0] == "Author One");
    assert(e.authors()[1] == "Author Two");
    assert(e.isbn() == "978-1234567890");
    assert(e.format() == EBookFormat::EPUB);
    assert(e.drmProtected());

    // Pricing must be inherited unchanged from ElectronicResource:
    // platformFee + unitPrice * seats = 500 + 100 * 3 = 800
    assert(e.costFor(3) == Money::of(800));

    std::ostringstream os;
    os << e;

    assert(os.str().find("access url: https://example.com/ebook") != std::string::npos);
    assert(os.str().find("platform fee: 500.00") != std::string::npos);
    assert(os.str().find("authors: Author One, Author Two") != std::string::npos);
    assert(os.str().find("isbn: 978-1234567890") != std::string::npos);
    assert(os.str().find("format: EPUB") != std::string::npos);
    assert(os.str().find("drm protected: yes") != std::string::npos);

    assert(std::string(ebookFormatName(EBookFormat::PDF)) == "PDF");
    assert(std::string(ebookFormatName(EBookFormat::EPUB)) == "EPUB");
    assert(std::string(ebookFormatName(EBookFormat::HTML)) == "HTML");
}


void testAudioBookAndThesis() {
    AudioBook audio("A1", "C++ Audio Guide", "Narrator One", 180,
                    "Tech Audio", 2026, Money::of(80),
                    "https://audio.example/cpp",
                    LicenseModel::AnnualSubscription,
                    Money::of(200));

    assert(audio.narrator() == "Narrator One");
    assert(audio.durationMinutes() == 180);

    // AudioBook inherits ElectronicResource behavior.
    assert(audio.category() == ResourceCategory::ElectronicResource);
    assert(audio.isDigital());
    assert(audio.costFor(3) == Money::of(440));  // 200 + 80 * 3

    std::ostringstream audioOut;
    audioOut << audio;
    assert(audioOut.str().find("access url: https://audio.example/cpp") != std::string::npos);
    assert(audioOut.str().find("narrator: Narrator One") != std::string::npos);
    assert(audioOut.str().find("duration: 180 minutes") != std::string::npos);

    Thesis thesis("T1", "Machine Learning for Search",
                  "Example University", "M.Tech", "Dr. Sharma",
                  "Example University", 2026);

    assert(thesis.university() == "Example University");
    assert(thesis.degree() == "M.Tech");
    assert(thesis.supervisor() == "Dr. Sharma");

    // Thesis defaults to free of cost.
    assert(thesis.unitPrice() == Money::of(0));
    assert(thesis.costFor(2) == Money::of(0));

    // Thesis uses the print/book category in this design.
    assert(thesis.category() == ResourceCategory::Book);
    assert(!thesis.isDigital());

    std::ostringstream thesisOut;
    thesisOut << thesis;
    assert(thesisOut.str().find("university: Example University") != std::string::npos);
    assert(thesisOut.str().find("degree: M.Tech") != std::string::npos);
    assert(thesisOut.str().find("supervisor: Dr. Sharma") != std::string::npos);
}

