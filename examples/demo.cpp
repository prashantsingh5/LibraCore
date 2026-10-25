// Demo: builds a small catalog, sets a budget with per-category quotas,
// and runs a batch of purchase requests through the acquisition manager.

#include <iostream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    Catalog catalog;

    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    catalog.emplace<Book>("B002", "The C++ Programming Language",
                          std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                          "Addison-Wesley", 2013, Money::of(1200), 4, Binding::Hardcover);
    catalog.emplace<Journal>("J001", "ACM Computing Surveys", "0360-0300", 4,
                             "ACM", 2026, Money::of(300), 2);
    catalog.emplace<ElectronicResource>("R001", "IEEE Xplore Digital Library", "IEEE", 2026,
                                        Money::of(150), "https://ieeexplore.example",
                                        LicenseModel::AnnualSubscription, Money::of(2000));
    catalog.emplace<ElectronicResource>("R002", "MATLAB Campus Licence", "MathWorks", 2026,
                                        Money::of(400), "https://licensing.example/matlab",
                                        LicenseModel::Perpetual);
    catalog.emplace<EBook>("E001", "Effective Modern C++", std::vector<std::string>{"Scott Meyers"}, 
                            "978-1491903995", EBookFormat::EPUB, true, "O'Reilly Media", 2014,
                            Money::of(120), "https://ebooks.example/effective-modern-cpp", 
                            LicenseModel::AnnualSubscription, Money::of(300));
    catalog.emplace<AudioBook>("A001", "The Pragmatic Programmer Audiobook", "Narrator Example", 
                                720, "Audio Press", 2026, Money::of(90), "https://audio.example/pragmatic-programmer",
                                LicenseModel::AnnualSubscription, Money::of(250));

    catalog.emplace<Thesis>("T001", "Machine Learning for Information Retrieval", "Example University", "M.Tech",
                            "Dr. A. Sharma", "Example University", 2026);


    std::cout << "=== Catalog ===\n";
    for (const Resource* r : catalog.all()) std::cout << r->summary() << "\n";

    std::cout << "\n=== Details of R001 ===\n" << catalog.get("R001");
    std::cout << "\n=== Details of J001 ===\n" << catalog.get("J001");
    std::cout << "\n=== Details of E001 ===\n" << catalog.get("E001");
    std::cout << "\n=== Details of A001 ===\n" << catalog.get("A001");
    std::cout << "\n=== Details of T001 ===\n" << catalog.get("T001");

    Budget budget(Money::of(20000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(8000)});
    budget.setQuota(ResourceCategory::ElectronicResource, {40, Money::of(12000)});
    budget.setQuota(ResourceCategory::Journal, {2, Money::of(1500)});
    budget.setQuota(ResourceCategory::EBook, {5, Money::of(1000)});

    AcquisitionManager acq(catalog, budget);

    std::cout << "\n=== Quotes ===\n";
    std::cout << "4 copies of B002  = " << acq.quote("B002", 4) << "  (hardcover includes 20% surcharge)\n";
    std::cout << "20 seats of R001  = " << acq.quote("R001", 20) << "  (incl. platform fee)\n";
    std::cout << "1 copy of J001    = " << acq.quote("J001", 1) << "  (2-year subscription)\n";
    std::cout << "3 seats of E001   = " << acq.quote("E001", 3) << "  (incl. platform fee)\n";
    std::cout << "2 seats of A001   = " << acq.quote("A001", 2) << "  (incl. platform fee)\n";
    std::cout << "1 copy of T001    = " << acq.quote("T001", 1) << "  (free thesis)\n";
    std::cout << "\n=== Q5 Bulk Pricing ===\n";
    std::cout << "10 copies of B001 = " << acq.quote("B001", 10) << "  (10% print bulk discount)\n";
    std::cout << "10 copies of B002 = " << acq.quote("B002", 10) << "  (20% hardcover surcharge, then 10% bulk discount)\n";
    std::cout << "10 copies of J001 = " << acq.quote("J001", 10) << "  (2-year subscription with 10% print bulk discount)\n";
    std::cout << "60 seats of R001  = " << acq.quote("R001", 60) << "  (seats above 50 are half price)\n";

    acq.processBatch({
        {"B001", 4},   // 1800  ok
        {"B002", 4},   // 5760  ok: hardcover includes 20% surcharge        
        {"B001", 1},   // 450   rejected: book spend quota (440 left)
        {"R001", 20},  // 5000  ok
        {"R002", 25},  // 10000 rejected: e-resource unit quota (20 seats left)
        {"R002", 15},  // 6000  ok  -> e-resource spend 11000
        {"R002", 5},   // 2000  rejected: e-resource spend quota (1000 left)
        {"J001", 1},   // 600   ok: 300 x 1 copy x 2 years
        {"X999", 1},   // rejected: unknown id
    });

    std::cout << "\n=== Acquisition report ===\n";
    acq.printReport(std::cout);

    std::cout << "\n=== Budget ===\n";
    budget.print(std::cout);

    std::cout << "\n=== Holdings ===\n";
    for (const Resource* r : catalog.all())
        std::cout << "  " << r->id() << ": " << catalog.holdings(r->id())
                  << (r->isDigital() ? " seats" : " copies") << "\n";

    // Direct purchase: errors are reported with exceptions
    std::cout << "\n=== Direct purchase that breaks a quota ===\n";
    try {
        acq.purchase("B002", 1);
    } catch (const QuotaExceededError& e) {
        std::cout << "QuotaExceededError: " << e.what() << "\n";
    }
    
    std::cout << "\n=== Q6 Taxes ===\n";

    Catalog taxCatalog;

    taxCatalog.emplace<Book>(
        "TB001", "Tax Demo Book",
        std::vector<std::string>{"Demo Author"},
        "978-tax-demo", "Demo Publisher", 2026,
        Money::of(100));

    taxCatalog.emplace<ElectronicResource>(
        "TR001", "Tax Demo Database",
        "Demo Publisher", 2026,
        Money::of(10),
        "https://tax-demo.example",
        LicenseModel::AnnualSubscription,
        Money::of(50));

    Budget taxBudget(Money::of(1000));
    taxBudget.setQuota(ResourceCategory::Book,
                       {10, Money::of(500)});
    taxBudget.setQuota(ResourceCategory::ElectronicResource,
                       {20, Money::of(500)});

    // Print items are taxed at 10%; electronic items at 20%.
    AcquisitionManager taxAcq(
        taxCatalog, taxBudget, TaxRates{10, 20});

    std::cout << "Print tax rate: "
              << taxAcq.taxRates().printPercent << "%\n";
    std::cout << "Electronic tax rate: "
              << taxAcq.taxRates().electronicPercent << "%\n";

    std::cout << "1 copy of TB001: pre-tax "
              << taxCatalog.get("TB001").costFor(1)
              << ", post-tax " << taxAcq.quote("TB001", 1) << "\n";

    std::cout << "5 seats of TR001: pre-tax "
              << taxCatalog.get("TR001").costFor(5)
              << ", post-tax " << taxAcq.quote("TR001", 5) << "\n";

    taxAcq.purchase("TB001", 2);
    taxAcq.purchase("TR001", 5);

    std::cout << "\n=== Q6 Taxed Acquisition Report ===\n";
    taxAcq.printReport(std::cout);

    std::cout << "\n=== Q6 Taxed Budget ===\n";
    taxBudget.print(std::cout);
    
    std::cout << "\n=== Q7 Different-Title Quota ===\n";

    Catalog titleCatalog;

    titleCatalog.emplace<Book>(
        "QB001", "First Quota Book",
        std::vector<std::string>{"Author One"},
        "quota-isbn-1", "Quota Publisher", 2026,
        Money::of(100));

    titleCatalog.emplace<Book>(
        "QB002", "Second Quota Book",
        std::vector<std::string>{"Author Two"},
        "quota-isbn-2", "Quota Publisher", 2026,
        Money::of(100));

    titleCatalog.emplace<Book>(
        "QB003", "Third Quota Book",
        std::vector<std::string>{"Author Three"},
        "quota-isbn-3", "Quota Publisher", 2026,
        Money::of(100));

    Budget titleBudget(Money::of(5000));

    titleBudget.setQuota(
        ResourceCategory::Book,
        {20, Money::of(5000), 2});

    AcquisitionManager titleAcq(titleCatalog, titleBudget);

    std::cout << "Initial Book titles used: "
              << titleBudget.titlesUsed(ResourceCategory::Book)
              << "/2\n";

    titleAcq.purchase("QB001", 2);

    std::cout << "After buying QB001: "
              << titleBudget.titlesUsed(ResourceCategory::Book)
              << "/2 titles used\n";

    // Buying the same title again does not consume another title slot.
    titleAcq.purchase("QB001", 1);

    std::cout << "After buying QB001 again: "
              << titleBudget.titlesUsed(ResourceCategory::Book)
              << "/2 titles used\n";

    titleAcq.purchase("QB002", 1);

    std::cout << "After buying QB002: "
              << titleBudget.titlesUsed(ResourceCategory::Book)
              << "/2 titles used\n";

    std::string titleWhy;

    if (!titleAcq.canPurchase("QB003", 1, &titleWhy)) {
        std::cout << "QB003 rejected: "
                  << titleWhy << "\n";
    }

    std::cout << "\n=== Q7 Title-Quota Budget ===\n";
    titleBudget.print(std::cout);
    std::cout << "\n=== Q8 Cancellation ===\n";

    Catalog cancelCatalog;

    cancelCatalog.emplace<Book>(
        "CB001", "Cancellation Demo Book",
        std::vector<std::string>{"Demo Author"},
        "cancel-demo-isbn", "Demo Publisher", 2026,
        Money::of(100));

    cancelCatalog.emplace<Book>(
        "CB002", "Second Cancellation Demo Book",
        std::vector<std::string>{"Demo Author"},
        "cancel-demo-isbn-2", "Demo Publisher", 2026,
        Money::of(100));

    Budget cancelBudget(Money::of(1000));

    cancelBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000), 2});

    // 10% print tax also demonstrates that the exact post-tax
    // amount is refunded.
    AcquisitionManager cancelAcq(
        cancelCatalog, cancelBudget, TaxRates{10, 20});

    const int cancelOrderNo =
        cancelAcq.purchase("CB001", 2).orderNo;

    cancelAcq.purchase("CB002", 1);

    std::cout << "Before cancellation:\n";
    std::cout << "  CB001 holdings: "
              << cancelCatalog.holdings("CB001")
              << " copies\n";
    std::cout << "  Budget spent: "
              << cancelBudget.spent() << "\n";
    std::cout << "  Book titles used: "
              << cancelBudget.titlesUsed(ResourceCategory::Book)
              << "/2\n";

    const PurchaseRecord& cancelled =
        cancelAcq.cancel(cancelOrderNo);

    std::cout << "\nCancelled purchase order #"
              << cancelled.relatedOrderNo << "\n";

    std::cout << "After cancellation:\n";
    std::cout << "  CB001 holdings: "
              << cancelCatalog.holdings("CB001")
              << " copies\n";
    std::cout << "  Budget spent: "
              << cancelBudget.spent() << "\n";
    std::cout << "  Book titles used: "
              << cancelBudget.titlesUsed(ResourceCategory::Book)
              << "/2\n";

    std::cout << "\n=== Q8 Cancellation History ===\n";
    cancelAcq.printReport(std::cout);

    std::cout << "\n=== Q8 Budget After Refund ===\n";
    cancelBudget.print(std::cout);

        std::cout << "\n=== Q9 Department Budgets ===\n";

    Catalog deptCatalog;

    deptCatalog.emplace<Book>(
        "DB001", "Algorithms for Departments",
        std::vector<std::string>{"Demo Author"},
        "dept-demo-isbn-1", "Demo Publisher", 2026,
        Money::of(100));

    deptCatalog.emplace<Book>(
        "DB002", "Physics Computing",
        std::vector<std::string>{"Demo Author"},
        "dept-demo-isbn-2", "Demo Publisher", 2026,
        Money::of(200));

    Budget deptDefaultBudget(Money::of(5000));

    Budget csBudget(Money::of(1000));
    csBudget.setQuota(
        ResourceCategory::Book,
        {3, Money::of(500), 2});

    Budget physicsBudget(Money::of(2000));
    physicsBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1500), 3});

    AcquisitionManager deptAcq(
        deptCatalog, deptDefaultBudget);

    deptAcq.addDepartment(
        "Computer Science", csBudget);

    deptAcq.addDepartment(
        "Physics", physicsBudget);

    deptAcq.purchase(
        "DB001", 2, "Computer Science");

    deptAcq.purchase(
        "DB001", 3, "Physics");

    std::cout << "After purchasing DB001 for both departments:\n";

    std::cout << "  Computer Science spent: "
              << csBudget.spent() << "\n";

    std::cout << "  Physics spent: "
              << physicsBudget.spent() << "\n";

    std::cout << "  Global DB001 holdings: "
              << deptCatalog.holdings("DB001")
              << " copies\n";

    std::string deptWhy;

    if (!deptAcq.canPurchase(
            "DB001", 2,
            "Computer Science",
            &deptWhy)) {

        std::cout << "Computer Science request rejected: "
                  << deptWhy << "\n";
    }

    if (deptAcq.canPurchase(
            "DB002", 1,
            "Physics",
            &deptWhy)) {

        std::cout << "Physics can still buy DB002 independently.\n";
    }

    std::cout << "\n=== Q9 Department Acquisition Report ===\n";
    deptAcq.printReport(std::cout);

    std::cout << "\n=== Computer Science Budget ===\n";
    csBudget.print(std::cout);

    std::cout << "\n=== Physics Budget ===\n";
    physicsBudget.print(std::cout);

    std::cout << "\n=== Q10 Year-End Rollover ===\n";

    Budget currentYear(Money::of(1000));

    currentYear.setQuota(
        ResourceCategory::Book,
        {10, Money::of(800), 3});

    currentYear.setQuota(
        ResourceCategory::ElectronicResource,
        {20, Money::of(900), 4});

    currentYear.commit(
        ResourceCategory::Book,
        "RY001",
        2,
        Money::of(300));

    std::cout << "Current year total: "
              << currentYear.total() << "\n";

    std::cout << "Current year spent: "
              << currentYear.spent() << "\n";

    std::cout << "Current year unspent: "
              << currentYear.remaining() << "\n";

    const int carryPercent = 50;
    const Money newAllocation = Money::of(2000);

    Budget nextYear =
        currentYear.nextYear(
            newAllocation,
            carryPercent);

    std::cout << "Carry-forward percentage: "
              << carryPercent << "%\n";

    std::cout << "New allocation: "
              << newAllocation << "\n";

    std::cout << "Next year total budget: "
              << nextYear.total() << "\n";

    std::cout << "Next year spent at start: "
              << nextYear.spent() << "\n";

    std::cout << "Next year Book units used: "
              << nextYear.usageFor(ResourceCategory::Book).units
              << "\n";

    std::cout << "Next year Book titles used: "
              << nextYear.titlesUsed(ResourceCategory::Book)
              << "\n";

    std::cout << "\n=== Q10 Next-Year Budget ===\n";
    nextYear.print(std::cout);

    std::cout << "\n=== Q11 All-or-Nothing Batch ===\n";

    Catalog atomicCatalog;

    atomicCatalog.emplace<Book>(
        "AB001", "Atomic Batch Book One",
        std::vector<std::string>{"Demo Author"},
        "atomic-demo-isbn-1", "Demo Publisher", 2026,
        Money::of(100));

    atomicCatalog.emplace<Book>(
        "AB002", "Atomic Batch Book Two",
        std::vector<std::string>{"Demo Author"},
        "atomic-demo-isbn-2", "Demo Publisher", 2026,
        Money::of(100));

    Budget atomicBudget(Money::of(1000));

    atomicBudget.setQuota(
        ResourceCategory::Book,
        {3, Money::of(1000), 2});

    AcquisitionManager atomicAcq(
        atomicCatalog, atomicBudget);

    std::cout << "Attempting atomic batch: AB001 x2, AB002 x2\n";

    auto atomicResult =
        atomicAcq.processBatch(
            {
                {"AB001", 2},
                {"AB002", 2}
            },
            true);

    for (const auto& rec : atomicResult) {
        std::cout << "  "
                  << rec.resourceId << ": "
                  << (rec.approved ? "approved" : "rejected");

        if (!rec.approved) {
            std::cout << " - " << rec.reason;
        }

        std::cout << "\n";
    }

    std::cout << "After failed atomic batch:\n";

    std::cout << "  AB001 holdings: "
              << atomicCatalog.holdings("AB001")
              << "\n";

    std::cout << "  AB002 holdings: "
              << atomicCatalog.holdings("AB002")
              << "\n";

    std::cout << "  Budget spent: "
              << atomicBudget.spent()
              << "\n";

    std::cout << "\nAttempting valid atomic batch: "
              << "AB001 x1, AB002 x1\n";

    auto validAtomic =
        atomicAcq.processBatch(
            {
                {"AB001", 1},
                {"AB002", 1}
            },
            true);

    for (const auto& rec : validAtomic) {
        std::cout << "  "
                  << rec.resourceId << ": "
                  << (rec.approved ? "approved" : "rejected")
                  << "\n";
    }

    std::cout << "After successful atomic batch:\n";

    std::cout << "  AB001 holdings: "
              << atomicCatalog.holdings("AB001")
              << "\n";

    std::cout << "  AB002 holdings: "
              << atomicCatalog.holdings("AB002")
              << "\n";

    std::cout << "  Budget spent: "
              << atomicBudget.spent()
              << "\n";

    std::cout << "\n=== Q11 Atomic Batch Report ===\n";
    atomicAcq.printReport(std::cout);

        std::cout << "\n=== Q12 Vendors ===\n";

    Catalog vendorCatalog;

    auto& vendorBook =
        vendorCatalog.emplace<Book>(
            "VD001", "Vendor Selection Book",
            std::vector<std::string>{"Demo Author"},
            "vendor-demo-isbn",
            "Demo Publisher", 2026,
            Money::of(120));

    vendorBook.addVendor(
        "Campus Books", Money::of(100));

    vendorBook.addVendor(
        "Academic Supply", Money::of(80));

    vendorBook.addVendor(
        "University Store", Money::of(90));

    Budget vendorBudget(Money::of(2000));

    vendorBudget.setQuota(
        ResourceCategory::Book,
        {20, Money::of(2000), 5});

    AcquisitionManager vendorAcq(
        vendorCatalog,
        vendorBudget,
        TaxRates{10, 20});

    std::cout << "Listed unit price: "
              << vendorBook.unitPrice()
              << "\n";

    std::cout << "Vendor offers:\n";

    for (const auto& offer : vendorBook.vendors()) {
        std::cout << "  "
                  << offer.vendor
                  << ": "
                  << offer.price
                  << "\n";
    }

    const VendorOffer* cheapest =
        vendorBook.cheapestVendor();

    if (cheapest) {
        std::cout << "Cheapest vendor: "
                  << cheapest->vendor
                  << " @ "
                  << cheapest->price
                  << "\n";
    }

    std::cout << "Quote for 2 copies including tax: "
              << vendorAcq.quote("VD001", 2)
              << "\n";

    const PurchaseRecord& vendorOrder =
        vendorAcq.purchase("VD001", 2);

    std::cout << "Purchased from: "
              << vendorOrder.vendor
              << "\n";

    std::cout << "Pre-tax order cost: "
              << vendorOrder.preTaxCost
              << "\n";

    std::cout << "Post-tax order cost: "
              << vendorOrder.cost
              << "\n";

    std::cout << "\n=== Q12 Vendor Acquisition Report ===\n";
    vendorAcq.printReport(std::cout);

    std::cout << "\n=== Q13 Catalog Searches ===\n";

    Catalog searchCatalog;

    searchCatalog.emplace<Book>(
        "S001", "Clean Code",
        std::vector<std::string>{"Robert C. Martin"},
        "978-0132350884",
        "Prentice Hall",
        2008,
        Money::of(450));

    searchCatalog.emplace<Book>(
        "S002", "Design Patterns",
        std::vector<std::string>{
            "Erich Gamma",
            "Richard Helm",
            "Ralph Johnson",
            "John Vlissides"
        },
        "978-0201633610",
        "Addison-Wesley",
        1994,
        Money::of(600));

    searchCatalog.emplace<EBook>(
        "S003", "Effective Modern C++",
        std::vector<std::string>{"Scott Meyers"},
        "978-1491903995",
        EBookFormat::EPUB,
        true,
        "O'Reilly Media",
        2014,
        Money::of(120),
        "https://search.example/effective-modern-cpp",
        LicenseModel::AnnualSubscription,
        Money::of(0));

    searchCatalog.emplace<Journal>(
        "S004", "Journal of Software Engineering",
        "1234-5678",
        12,
        "Demo Press",
        2020,
        Money::of(200));

    searchCatalog.emplace<ElectronicResource>(
        "S005", "Modern Research Database",
        "Demo Publisher",
        2026,
        Money::of(50),
        "https://search.example/database");

    std::cout << "\nSearch by author: \"meyers\"\n";

    for (const Resource* r :
         searchCatalog.searchAuthor("meyers")) {
        std::cout << "  "
                  << r->id()
                  << " - "
                  << r->title()
                  << "\n";
    }

    std::cout << "\nSearch by ISBN: 978-0132350884\n";

    for (const Resource* r :
         searchCatalog.searchISBN(
             "978-0132350884")) {
        std::cout << "  "
                  << r->id()
                  << " - "
                  << r->title()
                  << "\n";
    }

    std::cout << "\nSearch by ISSN: 1234-5678\n";

    for (const Resource* r :
         searchCatalog.searchISSN(
             "1234-5678")) {
        std::cout << "  "
                  << r->id()
                  << " - "
                  << r->title()
                  << "\n";
    }

    std::cout << "\nCombined identifier search: "
              << "978-1491903995\n";

    for (const Resource* r :
         searchCatalog.searchIdentifier(
             "978-1491903995")) {
        std::cout << "  "
                  << r->id()
                  << " - "
                  << r->title()
                  << "\n";
    }

    std::cout << "\nPublication years 2010-2026:\n";

    for (const Resource* r :
         searchCatalog.searchYearRange(
             2010, 2026)) {
        std::cout << "  "
                  << r->id()
                  << " - "
                  << r->title()
                  << " ("
                  << r->year()
                  << ")\n";
    }

    std::cout << "\n=== Q14 Lending ===\n";

    Catalog lendingCatalog;

    lendingCatalog.emplace<Book>(
        "L001", "Library Lending Book",
        std::vector<std::string>{"Demo Author"},
        "lending-demo-isbn",
        "Demo Publisher",
        2026,
        Money::of(100));

    lendingCatalog.emplace<ElectronicResource>(
        "L002", "Online Research Service",
        "Demo Publisher",
        2026,
        Money::of(20),
        "https://lending.example/research");

    Budget lendingBudget(Money::of(5000));

    lendingBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(2000), 5});

    lendingBudget.setQuota(
        ResourceCategory::ElectronicResource,
        {10, Money::of(2000), 5});

    AcquisitionManager lendingAcq(
        lendingCatalog,
        lendingBudget);

    lendingAcq.purchase("L001", 3);
    lendingAcq.purchase("L002", 2);

    std::cout << "Initial availability:\n";

    std::cout << "  L001: "
              << lendingCatalog.available("L001")
              << " of "
              << lendingCatalog.holdings("L001")
              << " copies available\n";

    std::cout << "  L002: "
              << lendingCatalog.available("L002")
              << " of "
              << lendingCatalog.holdings("L002")
              << " electronic sessions available\n";

    lendingCatalog.lend("L001", 2);
    lendingCatalog.lend("L002", 2);

    std::cout << "\nAfter lending / starting sessions:\n";

    std::cout << "  L001 lent: "
              << lendingCatalog.lent("L001")
              << ", available: "
              << lendingCatalog.available("L001")
              << "\n";

    std::cout << "  L002 active sessions: "
              << lendingCatalog.lent("L002")
              << ", available sessions: "
              << lendingCatalog.available("L002")
              << "\n";

    try {
        lendingCatalog.lend("L002");
    } catch (const std::invalid_argument& e) {
        std::cout << "  Additional electronic session rejected: "
                  << e.what()
                  << "\n";
    }

    lendingCatalog.returnLoan("L001");
    lendingCatalog.returnLoan("L002");

    std::cout << "\nAfter one return / session end:\n";

    std::cout << "  L001 lent: "
              << lendingCatalog.lent("L001")
              << ", available: "
              << lendingCatalog.available("L001")
              << "\n";

    std::cout << "  L002 active sessions: "
              << lendingCatalog.lent("L002")
              << ", available sessions: "
              << lendingCatalog.available("L002")
              << "\n";

    std::cout << "\n=== Q15 Currency-Aware Money ===\n";

    Money usdExample =
        Money::of(125, 50, "USD");

    Money eurExample =
        Money::of(80, 0, "EUR");

    std::cout << "USD example: "
              << usdExample.currency()
              << " "
              << usdExample
              << "\n";

    std::cout << "EUR example: "
              << eurExample.currency()
              << " "
              << eurExample
              << "\n";

    Money usdExtra =
        Money::of(24, 50, "USD");

    Money usdTotal =
        usdExample + usdExtra;

    std::cout << "USD same-currency addition: "
              << usdTotal.currency()
              << " "
              << usdTotal
              << "\n";

    try {
        Money invalid =
            usdExample + eurExample;

        std::cout << invalid << "\n";
    } catch (const std::invalid_argument& e) {
        std::cout
            << "Mixed-currency addition rejected: "
            << e.what()
            << "\n";
    }

    std::cout
        << "\n--- Q15 USD Acquisition Workflow ---\n";

    Catalog currencyCatalog;

    auto& currencyBook =
        currencyCatalog.emplace<Book>(
            "USD001",
            "International Library Book",
            std::vector<std::string>{"Demo Author"},
            "usd-demo-isbn",
            "Demo Publisher",
            2026,
            Money::of(120, 0, "USD"));

    currencyBook.addVendor(
        "Global Books",
        Money::of(100, 0, "USD"));

    currencyBook.addVendor(
        "Academic Export",
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

    std::cout << "Budget currency: "
              << currencyBudget.total().currency()
              << "\n";

    std::cout << "Cheapest vendor quote for 2 copies: "
              << currencyAcq.quote("USD001", 2).currency()
              << " "
              << currencyAcq.quote("USD001", 2)
              << "\n";

    const PurchaseRecord& currencyOrder =
        currencyAcq.purchase("USD001", 2);

    std::cout << "Purchased from: "
              << currencyOrder.vendor
              << "\n";

    std::cout << "Pre-tax: "
              << currencyOrder.preTaxCost.currency()
              << " "
              << currencyOrder.preTaxCost
              << "\n";

    std::cout << "Post-tax: "
              << currencyOrder.cost.currency()
              << " "
              << currencyOrder.cost
              << "\n";

    std::cout << "Budget remaining: "
              << currencyBudget.remaining().currency()
              << " "
              << currencyBudget.remaining()
              << "\n";
            
    std::cout << "\n=== V1 Combined Search ===\n";
    searchCatalog.emplace<Book>("S006", "Clean Architecture", std::vector<std::string>{"Robert C. Martin"},
                                "978-0134494166", "Prentice Hall", 2017, Money::of(550));
    std::cout << "Books by Martin since 2000, newest first:\n";
    SearchQuery query;
    query.category = ResourceCategory::Book; query.author = "Martin";
    query.fromYear = 2000; query.sort = SearchSort::YearDescending;
    for (const auto* r : searchCatalog.search(query)) std::cout << r->summary() << "\n";

    std::cout << "\n=== V1 Lending Records ===\n";
    auto namedLoan = lendingCatalog.borrow("L001", "student-42");
    const auto days = std::chrono::duration_cast<std::chrono::hours>(namedLoan.dueDate - namedLoan.borrowDate).count()/24;
    std::cout << namedLoan.loanId << ": " << namedLoan.borrowerId << " borrowed " << namedLoan.resourceId
              << "; due in " << days << " days\n";
    lendingCatalog.returnLoan(namedLoan.loanId, std::chrono::system_clock::now());
    std::cout << "Returned; borrower history contains " << lendingCatalog.loansForUser("student-42").size() << " loan(s)\n";

    std::cout << "\n=== V1 Vendor Selection ===\n";
    Catalog v1Catalog;
    auto& v1Book = v1Catalog.emplace<Book>("V1", "Domain Engines", std::vector<std::string>{"Author"}, "v1-isbn", "Press", 2026, Money::of(120));
    v1Book.addVendor({"Economy Books", Money::of(80), "economy", 4.5, 7});
    v1Book.addVendor({"Express Books", Money::of(100), "express", 4.8, 1});
    Budget v1Budget(Money::of(1000));
    v1Budget.setQuota(ResourceCategory::Book, {10, Money::of(1000), 3});
    AcquisitionManager v1Acq(v1Catalog, v1Budget);
    v1Acq.setConfig({{10,20}, VendorSelection::Cheapest});
    std::cout << "Cheapest quote: " << v1Acq.quote("V1",1).toCurrencyString()
              << "; fastest quote: " << v1Acq.quote("V1",1,VendorSelection::Fastest).toCurrencyString()
              << "; highest-rated quote: " << v1Acq.quote("V1",1,VendorSelection::HighestRated).toCurrencyString() << "\n";
    PurchaseRequest request{"V1",2}; request.vendorSelection = VendorSelection::Fastest;
    auto v1Order = v1Acq.submit(request);
    std::cout << "Approved request #" << request.orderNo << " from " << v1Order.vendor << "\n";

    std::cout << "\n=== V1 Reporting ===\n";
    const auto budgetSummary = v1Budget.summary();
    const auto categorySummary = v1Budget.categorySummary(ResourceCategory::Book);
    std::cout << "Budget used: " << budgetSummary.percentageUsed << "%; remaining " << budgetSummary.remaining.toCurrencyString() << "\n";
    std::cout << "Book units: " << categorySummary.unitsUsed << "; titles: " << categorySummary.titlesUsed << "\n";
    const auto lendingSummary = lendingCatalog.lendingSummary();
    std::cout << "Holdings: " << lendingSummary.totalHoldings << "; borrowed: " << lendingSummary.borrowed
              << "; active loans: " << lendingSummary.activeLoans << "\n";
    for (const auto& d : deptAcq.departmentSummaries()) std::cout << "Department " << (d.department.empty()?"default":d.department) << ": " << d.spent.toCurrencyString() << " spent\n";
    v1Acq.cancel(v1Order.orderNo);
    std::cout << "Acquisition counts: " << v1Acq.summary().approved << " approved, " << v1Acq.summary().cancelled << " cancelled\n";
    std::cout << "\n=== V1 Event Records ===\n";
    const auto printEvent = [](const Event& e) {
        const char* action = "";
        switch (e.type) {
            case EventType::Purchase: action = "Purchase"; break;
            case EventType::Cancellation: action = "Cancellation"; break;
            case EventType::Borrow: action = "Borrow"; break;
            case EventType::Return: action = "Return"; break;
        }
        std::cout << action << " " << e.resourceId << " x" << e.quantity
                  << " actor=" << (e.actor.empty() ? "default budget" : e.actor)
                  << " reference=" << e.referenceId << " time(epoch seconds)="
                  << std::chrono::duration_cast<std::chrono::seconds>(e.timestamp.time_since_epoch()).count() << "\n";
    };
    for (const auto& e : v1Catalog.events()) printEvent(e);
    for (const auto& e : lendingCatalog.events())
        if (e.referenceId == namedLoan.loanId &&
            (e.type == EventType::Borrow || e.type == EventType::Return)) printEvent(e);

    return 0;
}
