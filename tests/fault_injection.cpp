// Isolated allocation-failure checks: never linked into the library or demo.
#include "TestSupport.h"
#include <cstdlib>
#include <new>
#include <functional>
namespace { long allocationsBeforeFailure = -1; }
void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) throw std::bad_alloc();
    if (allocationsBeforeFailure > 0) --allocationsBeforeFailure;
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    try { return ::operator new(size); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    try { return ::operator new[](size); } catch (...) { return nullptr; }
}
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
struct Fixture {
    Catalog c;
    Budget b{Money::of(1000)};
    AcquisitionManager a{c,b};
    Fixture() {
        c.emplace<Book>("B", "Book", std::vector<std::string>{}, "", "", 2000, Money::of(10));
        c.addHoldings("B", 5);
    }
};
int main() {
    int failurePoints = 0;
    for (int scenario = 0; scenario < 7; ++scenario) {
        bool completed = false;
        for (long failAt=0; failAt<1000; ++failAt) {
            Fixture f;
            std::string loanId;
            int order = 0;
            if (scenario == 2) loanId = f.c.borrow("B", "user", 2).loanId;
            if (scenario == 3) order = f.a.purchase("B",2).orderNo;
            if (scenario == 6) f.c.lend("B",2);
            const auto spent = f.b.spent();
            const auto holdings = f.c.holdings("B");
            const auto lent = f.c.lent("B");
            const auto events = f.c.events().size();
            const auto history = f.a.history().size();
            const auto loans = f.c.activeLoans().size();
            PurchaseRequest request{"B",2};
            const std::vector<PurchaseRequest> batch{{"B",1},{"B",2}};
            allocationsBeforeFailure = failAt;
            bool threw = false;
            try {
                switch(scenario) {
                    case 0: f.a.purchase("B",2); break;
                    case 1: f.c.borrow("B","user",2); break;
                    case 2: f.c.returnBorrowed(loanId,1); break;
                    case 3: f.a.cancel(order); break;
                    case 4: f.a.processBatch(batch,true); break;
                    case 5: f.a.submit(request); break;
                    case 6: f.c.returnLoan("B",2); break;
                }
            } catch (const std::bad_alloc&) { threw = true; }
            allocationsBeforeFailure = -1;
            if (!threw) { completed = true; break; }
            ++failurePoints;
            assert(f.b.spent() == spent && f.c.holdings("B") == holdings && f.c.lent("B") == lent);
            assert(f.c.events().size() == events && f.a.history().size() == history);
            assert(f.c.activeLoans().size() == loans);
            assert(request.status == AcquisitionStatus::Requested && request.orderNo == 0);
        }
        assert(completed);
    }
    std::cout << "7 transaction scenarios passed " << failurePoints << " allocation-failure points.\n";
}
