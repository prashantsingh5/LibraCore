#include "TestSupport.h"
void testV1Lending() {
    Catalog c;
    c.emplace<Book>("B", "Book", std::vector<std::string>{}, "", "", 2000, Money::of(10));
    c.addHoldings("B", 5);
    const Timestamp t = Timestamp{} + std::chrono::hours(1000);
    auto l = c.borrow("B", "U1", 2, t);
    assert(l.dueDate == t + std::chrono::hours(14*24));
    c.setLendingConfig({7});
    auto l2 = c.borrow("B", "U2", 1, t);
    assert(l2.dueDate == t + std::chrono::hours(7*24));
    auto l3 = c.borrow("B", "U1", 1, t, t + std::chrono::hours(1));
    assert(l3.dueDate == t + std::chrono::hours(1));
    assert(c.activeLoans().size() == 3 && c.loansForUser("U1").size() == 2);
    assert(c.loansForResource("B").size() == 3);
    auto before = c.events().size();
    assertThrows<std::invalid_argument>([&] { c.borrow("B", " "); });
    assertThrows<std::invalid_argument>([&] { c.borrow("B", "U3", 2); });
    assertThrows<std::invalid_argument>([&] { c.borrow("B", "U3", 0); });
    assertThrows<std::invalid_argument>([&] { c.borrow("B", "U3", 1, t, t); });
    assertThrows<std::invalid_argument>([&] { c.returnBorrowed(l.loanId, 3, t); });
    assertThrows<std::invalid_argument>([&] { c.returnBorrowed(l.loanId, 1, t - std::chrono::hours(1)); });
    assertThrows<std::invalid_argument>([&] { c.returnLoan("B"); });
    assertThrows<std::invalid_argument>([&] { c.remove("B"); });
    assertThrows<std::invalid_argument>([&] { c.setLendingConfig({0}); });
    assert(c.events().size() == before && c.available("B") == 1);
    c.returnBorrowed(l.loanId, 1, t);
    assert(c.loan(l.loanId).outstanding() == 1 && !c.loan(l.loanId).returnDate);
    c.returnLoan(l.loanId, t + std::chrono::hours(2));
    assert(c.loan(l.loanId).status == LoanStatus::Returned);
    assert(c.loan(l.loanId).returnDate == t + std::chrono::hours(2));
    assert(c.lendingSummary().activeLoans == 2);
    assert(c.lendingSummary().borrowed == 2 && c.lendingSummary().available == 3);
    c.lend("B", 2); c.returnLoan("B", 2);
    assert(c.lent("B") == 2); // legacy returns cannot close named loans
    assertThrows<std::invalid_argument>([&] { c.returnBorrowed(l.loanId, 1); });
    Catalog empty; assert(empty.lendingSummary().totalHoldings == 0);
}
