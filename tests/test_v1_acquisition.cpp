#include "TestSupport.h"
#include <limits>
void testV1Vendors() {
    Book b("B", "Book", {}, "", "", 2020, Money::of(100));
    b.addVendor({"Slow", Money::of(50), "S", 4, 8});
    b.addVendor({"Fast", Money::of(80), "F", 3, 1});
    b.addVendor({"Rated", Money::of(70), "R", 5, 3});
    assert(b.selectVendor(VendorSelection::Cheapest)->vendorId == "S");
    assert(b.selectVendor(VendorSelection::Fastest)->vendorId == "F");
    assert(b.selectVendor(VendorSelection::HighestRated)->vendorId == "R");
    b.addVendor({"Tie Z", Money::of(50), "Z", 4, 8});
    b.addVendor({"Tie A", Money::of(50), "A", 4, 8});
    assert(b.cheapestVendor()->vendorId == "A");
    assertThrows<std::invalid_argument>([&]{ b.addVendor({"X", Money{}, "X", -1, 0}); });
    assertThrows<std::invalid_argument>([&]{ b.addVendor({"X", Money{}, "X", 6, 0}); });
    assertThrows<std::invalid_argument>([&]{ b.addVendor({"X", Money{}, "X", std::numeric_limits<double>::quiet_NaN(), 0}); });
    assertThrows<std::invalid_argument>([&]{ b.addVendor({"X", Money{}, "X", 1, -1}); });
    assertThrows<std::invalid_argument>([&]{ b.addVendor({"X", Money{}, " ", 1, 0}); });
    assertThrows<std::invalid_argument>([&]{ b.addVendor({"X", Money{}, "A", 1, 0}); });
    assertThrows<std::invalid_argument>([&]{ b.addVendor({"X", Money::of(1,0,"USD"), "X", 1, 0}); });
    assertThrows<std::invalid_argument>([&]{ b.setUnitPrice(Money::of(1,0,"USD")); });
    assert(b.vendors().size() == 5);
}
void testV1Acquisition() {
    Catalog c;
    auto& b = c.emplace<Book>("B", "Book", std::vector<std::string>{}, "", "", 2020, Money::of(100));
    b.addVendor({"Slow", Money::of(50), "S", 4, 8});
    b.addVendor({"Fast", Money::of(80), "F", 3, 1});
    Budget budget(Money::of(1000)), department(Money::of(500));
    AcquisitionManager a(c, budget);
    a.addDepartment("CS", department);
    a.setConfig({{10,20}, VendorSelection::Fastest});
    assert(a.quote("B", 1) == Money::of(88));
    assert(a.quote("B", 1, VendorSelection::Cheapest) == Money::of(55));
    PurchaseRequest req{"B", 2, "CS"};
    assert(req.status == AcquisitionStatus::Requested);
    auto rec = a.submit(req);
    assert(rec.vendorOffer->vendorId == "F" && rec.vendorOffer->deliveryDays == 1);
    assert(req.orderNo == rec.orderNo && req.status == AcquisitionStatus::Approved);
    assertThrows<std::invalid_argument>([&]{ a.submit(req); });
    auto loan = c.borrow("B", "User");
    assertThrows<std::invalid_argument>([&]{ a.cancel(rec.orderNo); });
    assert(department.spent() == Money::of(176));
    c.returnLoan(loan.loanId, std::chrono::system_clock::now());
    b.setUnitPrice(Money::of(999));
    auto cancelled = a.cancel(rec.orderNo);
    assert(cancelled.status == AcquisitionStatus::Cancelled && cancelled.department == "CS");
    assert(cancelled.vendorOffer->vendorId == "F" && cancelled.cost == Money::of(176));
    assert(a.orderStatus(rec.orderNo) == AcquisitionStatus::Cancelled);
    assert(a.history().front().status == AcquisitionStatus::Approved); // immutable action history
    assertThrows<std::invalid_argument>([&]{ a.cancel(rec.orderNo); });
    PurchaseRequest bad{"B", 999, "CS"}; a.submit(bad);
    assert(bad.status == AcquisitionStatus::Rejected && !bad.reason.empty() && bad.orderNo > 0);
    assert(a.summary().approved == 1 && a.summary().rejected == 1 && a.summary().cancelled == 1);
    assert(a.summary().totalPostTax == Money{});
    auto events = c.events();
    assert(events.size() == 4);
    assert(events[0].type == EventType::Purchase && events[1].type == EventType::Borrow);
    assert(events[2].type == EventType::Return && events[3].type == EventType::Cancellation);
    assert(events[0].actor == "CS" && events[1].actor == "User");
    for (std::size_t i=1; i<events.size(); ++i) assert(events[i-1].timestamp <= events[i].timestamp);
    assertThrows<std::invalid_argument>([&]{ a.setConfig({{-1,0}, VendorSelection::Cheapest}); });
    assert(a.config().vendorSelection == VendorSelection::Fastest && a.taxRates().printPercent == 10);
    assertThrows<std::invalid_argument>([&]{ a.addDepartment(" \t", department); });
}
void testV1Transactions() {
    Catalog c;
    c.emplace<Book>("B", "Book", std::vector<std::string>{}, "", "", 2020, Money::of(100));
    Budget b(Money::of(150)); AcquisitionManager a(c,b);
    a.addDepartment("alias", b);
    auto rejected = a.processBatch({{"B",1},{"B",1,"alias"}}, true);
    assert(!rejected[0].approved && !rejected[1].approved);
    assert(b.spent().isZero() && c.holdings("B") == 0 && c.events().empty());
    c.addHoldings("B", std::numeric_limits<int>::max());
    assertThrows<std::overflow_error>([&]{ a.purchase("B",1); });
    assert(b.spent().isZero() && a.history().size() == 2);
    assert(!a.canPurchase("B",1));
    auto overflow = a.processBatch({{"B",1}}, true);
    assert(!overflow[0].approved && b.spent().isZero());
    auto invalid = a.processBatch({{" ",1},{"B",0},{"missing",1}}, true);
    for (const auto& r : invalid) assert(r.status == AcquisitionStatus::Rejected && !r.reason.empty());
    c.addHoldings("B", -std::numeric_limits<int>::max());
    c.emplace<Book>("USD", "Dollar", std::vector<std::string>{}, "", "", 2020, Money::of(1,0,"USD"));
    auto mixed = a.processBatch({{"B",1},{"USD",1}}, true);
    assert(!mixed[0].approved && !mixed[1].approved && c.events().empty());
    auto independent = a.processBatch({{"USD",1},{"B",1}});
    assert(!independent[0].approved && independent[1].approved);
    assert(b.spent() == Money::of(100) && c.events().size() == 1);
}
