# LibraCore V1 — completion report

Verified 24 October 2026. Implemented directly in the uploaded C++17 project.
The original archive layout (`oopd-a2-main/MT26076`) is retained.

## Outcome

All 18 requested improvements are implemented. The original 18 test function
bodies are preserved (apart from removing `static` to split translation units).
The main executable now runs 26 test functions. Both CTest targets pass.
Clean configure/build, original plus V1 demo, and 10K/100K/1M benchmarks pass.
Compiler output contains no warnings. The archive excludes build directories,
compiled binaries, temporary scripts and generated datasets.

## Baseline and final verification

CMake was initially absent; installed it in the execution environment, then
configured and built the original project before refactoring. The baseline
18-function test suite and demo passed. A direct GCC build also passed.
Baseline outputs are `docs/baseline-tests.txt` and `docs/baseline-demo.txt`.

Final verification uses GCC 13.3.0, C++17, CMake 4.4.4, Linux x86_64, with no
CMake build type selected (no optimization flag added). Evidence:

| Check | Result | Evidence |
|---|---|---|
| Clean configure in a new build directory | Passed | docs/clean-configure.txt |
| Build all targets with warnings | Passed, zero warnings | docs/clean-build.txt |
| Main regression suite | 26 functions passed: 18 original + 8 new | docs/test-results.txt |
| Allocation-failure rollback | 7 scenarios, 58 allocation points passed | docs/test-results.txt |
| CTest | 2/2 targets passed | docs/test-results.txt |
| Original and V1 demo | Exit 0 | docs/demo-output.txt |
| 10K, 100K, 1M benchmarks | All exit 0 | docs/benchmark-*.txt |
| AddressSanitizer + UndefinedBehaviorSanitizer | 2/2 targets passed | docs/sanitizer-final.txt |
| Original test preservation | All 18 bodies matched the uploaded source | docs/static-checks.txt |

Sanitizer detail: the first run exposed a missing nothrow allocator overload in
the isolated fault-injection test. That test harness was corrected, and the
sanitizer rerun passed with allocation-mismatch checking enabled. LeakSanitizer
cannot operate under this environment's tracing; only leak detection was disabled
(`ASAN_OPTIONS=detect_leaks=0`). Leak-freedom is therefore not claimed. The first
run's diagnostic is retained in `docs/sanitizer-results.txt` for transparency.
Windows commands are documented but not tested on Windows.

## Per-improvement changes

Paths below are relative to `MT26076`. Every row's result is **implemented and
verified**; the listed tests ran successfully.

| Improvement | What changed | Why | Files modified/added | Functions/classes | Tests / evidence |
|---|---|---|---|---|---|
| 1. Responsibilities | Separated request preparation, vendor pricing, tax application, budget lookup and staged transaction commit; removed repeated batch pricing/commit paths. | To keep acquisition behavior readable and atomic. | include/bookmgmt/Acquisition.h; src/Acquisition.cpp | prepare, priceForPurchase, applyTax, commitRecords, budgetForDepartment | testV1Acquisition, testV1Transactions; fault injection |
| 2. ID validation | Added one nonblank-ID helper used for resources, borrowers, loan IDs, vendors, budgets and department registration. Numeric order IDs remain positive integers. | Reject empty/whitespace identifiers consistently while keeping string IDs. | include/bookmgmt/Validation.h; src/Resource.cpp, Budget.cpp, Catalog.cpp, Lending.cpp, Acquisition.cpp | validId, requireId | testV1Search, testV1Lending, testV1Vendors, testV1Transactions, testV1Reports |
| 3. Search | Added AND-combined optional filters, inclusive/one-sided years and six sort modes. Legacy searches forward into shared query logic. | Support realistic queries without leaking internal containers. | include/bookmgmt/Catalog.h; src/Catalog.cpp | SearchQuery, SearchSort, Catalog::search | testV1Search; original testCatalogSearches |
| 4. Lending model | Added borrower-aware records, partial/full returns, active/user/resource queries and immutable returned snapshots. Anonymous compatibility loans share availability state. | Answer who borrowed what and retain return history. | include/bookmgmt/Lending.h, Catalog.h; src/Lending.cpp, Catalog.cpp | Loan, LoanStatus, borrow, returnBorrowed, returnLoan(timestamp), loan, activeLoans, loansForUser, loansForResource | testV1Lending; original testLending; fault injection |
| 5. Due-date rule | Added a configurable 14-day default and explicit due-date override with chrono timestamps. | Centralize duration policy and permit deterministic caller-supplied dates. | include/bookmgmt/Lending.h; src/Lending.cpp | LendingConfig, setLendingConfig, lendingConfig | testV1Lending |
| 6. Acquisition status | Introduced Requested/Approved/Rejected/Cancelled enum; retained approved/type compatibility fields and immutable action history. Current order status resolves cancellations. | Clarify action outcome without breaking original reports. | include/bookmgmt/Acquisition.h; src/Acquisition.cpp | AcquisitionStatus, orderStatus | testV1Acquisition; original testCancellation |
| 7. Request record | Extended PurchaseRequest with status, reason, order number and optional vendor mode; submit updates it and prevents resubmission. | Expose a simple request/outcome workflow for future clients. | include/bookmgmt/Acquisition.h; src/Acquisition.cpp | PurchaseRequest fields, submit | testV1Acquisition; submit allocation-failure scenario |
| 8. Vendor metadata | Added vendor ID, finite 0..5 rating and nonnegative delivery days; validate unique IDs and same-currency offers. Orders retain the selected offer snapshot. | Make supplier choice and later audit meaningful. | include/bookmgmt/Resource.h, Acquisition.h; src/Resource.cpp, Acquisition.cpp | VendorOffer fields, addVendor(VendorOffer) | testV1Vendors, testV1Acquisition |
| 9. Vendor modes | Added Cheapest, Fastest and HighestRated; configurable default and per-quote/purchase/request override. Documented deterministic tie order. | Allow transparent price/delivery tradeoffs while preserving cheapest default. | include/bookmgmt/Resource.h, Acquisition.h; src/Resource.cpp, Acquisition.cpp | VendorSelection, selectVendor, quote(mode), purchase(mode) | testV1Vendors, testV1Acquisition; original testVendors |
| 10. Reporting | Added structured overall/category/department/acquisition/lending summaries; kept console reports. | Expose values without console parsing. | include/bookmgmt/Budget.h, Acquisition.h, Lending.h; src/Budget.cpp, Acquisition.cpp, Lending.cpp | BudgetSummary, CategorySummary, DepartmentSummary, AcquisitionSummary, LendingSummary | testV1Reports, testV1Acquisition, testV1Lending |
| 11. Utilization | Added budget and category unit/title/spend percentages; unlimited is nullopt and zero-unused quota is 0%. Reject quota reductions below usage. | Prevent division by zero and ambiguous unlimited results. | include/bookmgmt/Budget.h; src/Budget.cpp | summary, categorySummary | testV1Reports |
| 12. Audit history | Added one catalog-level chronological event view for successful purchases, cancellations, borrows and returns, with actor/time/reference. | Connect significant actions across acquisition and lending. | include/bookmgmt/History.h, Catalog.h; src/Acquisition.cpp, Lending.cpp | Event, EventType, Catalog::events | testV1Acquisition, testV1Lending; fault injection |
| 13. Configuration | Added small LendingConfig and AcquisitionConfig objects; retained TaxRates constructor and setter. | Centralize defaults without external configuration infrastructure. | include/bookmgmt/Lending.h, Acquisition.h; src/Lending.cpp, Acquisition.cpp | setConfig, config, setLendingConfig | testV1Acquisition, testV1Lending |
| 14. Money polish | Added explicit currency-prefixed display; preserved numeric output and code normalization. Also checked arithmetic and percentage overflow and INT64_MIN formatting. | Keep existing money design while avoiding undefined overflow on large inputs. | include/bookmgmt/Money.h; src/Money.cpp, Resource.cpp, Book.cpp, MT26076_Journal.cpp, ElectronicResource.cpp, Budget.cpp, Acquisition.cpp | toCurrencyString, scaledRatio, checked operators | testV1Money; original testMoney/testCurrencyAwareMoney |
| 15. Search scalability preparation | Kept query results as const resource pointers and returned loan/report values. Default map order avoids unnecessary sorting. | Prepare a clean client-facing API without prematurely adding a database layer. | include/bookmgmt/Catalog.h; src/Catalog.cpp | search; existing lookup and search APIs | testV1Search; benchmark runs |
| 16. Data generator | Added deterministic mixed Books/Journals/ElectronicResources with unique IDs and configurable count. | Provide repeatable realistic-size search workloads. | benchmarks/SyntheticCatalog.h | benchmark::syntheticId, generateCatalog | testGenerator (0 and 1003 resources); 10K/100K/1M runs |
| 17. Benchmarks | Added independent executable measuring generation, 10000 lookups and four searches; prints ms and match counts. | Record actual scaling behavior outside normal unit tests. | benchmarks/benchmark_catalog.cpp; CMakeLists.txt | bookmgmt_benchmark target; measure helper | 10K/100K/1M executable runs; invalid argument checks |
| 18. Test organization | Split all 18 original function bodies unchanged into logical files; added eight V1 functions and isolated allocation-failure tests. Enabled warnings on all targets and assertions in Release tests. | Make regressions explainable and keep tests effective in all configurations. | tests/TestSupport.h; tests/test_*.cpp; tests/fault_injection.cpp; CMakeLists.txt | 26-function runner; bookmgmt_fault_tests target | Both CTest targets pass; original bodies compared against uploaded ZIP |

## Resulting responsibilities

| Component | Resulting responsibility |
|---|---|
| Resource hierarchy | Own resource metadata and polymorphic pre-tax pricing. Journal/EBook/AudioBook/Thesis and original category rules are retained. |
| Catalog | Own resources and holdings; provide queries, sorting, loan facade and shared audit timeline. |
| Lending | Dedicated Lending.h/Lending.cpp records and operations on Catalog; one source of availability truth. Partial returns retain original quantity. |
| Budget | Post-tax spending/quota checks, title counts, refunds, rollover, snapshots and utilization. Standalone commits stage changes as well. |
| AcquisitionManager | Prepare/price/validate requests; select department budgets; stage complete commits; preserve cancellation and atomic-batch semantics. |
| Vendor handling | Resource-owned validated offers with deterministic ranking; acquisition stores a snapshot of the selected offer. |
| Reporting | Plain value structures, independent of existing text output; unlimited allowances represented explicitly with optional values. |
| Money | Original fixed-point/currency model, explicit currency display and checked arithmetic; no conversion. |

## New public function summary

| Function / API | Class | Header | Purpose |
|---|---|---|---|
| validId / requireId | free functions | Validation.h | Consistent nonblank identifier checks |
| search(SearchQuery) | Catalog | Catalog.h | Combined filters and sorting |
| Catalog(LendingConfig), setLendingConfig / lendingConfig | Catalog | Catalog.h | Configure default loan duration |
| borrow(resource, borrower, quantity, when, due) | Catalog | Catalog.h | Create a named loan or electronic session |
| returnBorrowed(loanId, quantity, when) | Catalog | Catalog.h | Partial or complete named return |
| returnLoan(loanId, Timestamp) | Catalog | Catalog.h | Return all remaining units of a named loan |
| loan(loanId) | Catalog | Catalog.h | Read one loan snapshot |
| activeLoans | Catalog | Catalog.h | Read all currently active records |
| loansForUser / loansForResource | Catalog | Catalog.h | Read borrower/resource histories |
| lendingSummary | Catalog | Catalog.h | Holdings, borrowed, available and active counts |
| events | Catalog | Catalog.h | Unified chronological audit snapshots |
| addVendor(VendorOffer) | Resource | Resource.h | Register validated metadata-rich offer |
| selectVendor(mode) | Resource | Resource.h | Deterministic cheapest/fastest/highest-rated selection |
| setConfig / config | AcquisitionManager | Acquisition.h | Tax and default vendor configuration |
| quote(id, quantity, mode) | AcquisitionManager | Acquisition.h | Price using a specified vendor strategy |
| purchase(id, quantity, mode, department) | AcquisitionManager | Acquisition.h | Buy with an explicit strategy |
| submit(PurchaseRequest&) | AcquisitionManager | Acquisition.h | Process and update a request outcome |
| orderStatus(orderNo) | AcquisitionManager | Acquisition.h | Resolve current order state after cancellation |
| summary | AcquisitionManager | Acquisition.h | Historical action counts and net monetary totals |
| departmentSummaries | AcquisitionManager | Acquisition.h | Per-budget values, default plus named departments |
| summary / categorySummary | Budget | Budget.h | Budget and category usage snapshots |
| swap(Budget&) noexcept | Budget | Budget.h | Nonthrowing transactional state publication |
| toCurrencyString | Money | Money.h | Explicit currency-prefixed output |
| scaledRatio | Money | Money.h | Checked fixed-point ratio, truncating minor units |
| generateCatalog / syntheticId | benchmark namespace | benchmarks/SyntheticCatalog.h | Repeatable synthetic resource generation |

## Regression coverage

Original cases still verify Journal subscriptions/ISSN; EBook authors/ISBN/format/DRM;
AudioBook and Thesis; 20% hardcover surcharge; 10% print bulk discount; half-price
electronic seats beyond 50; separate taxes and post-tax quotas; title quotas;
cancellation/refund/holdings/history; departments; rollover; atomic batches;
cheapest vendors; catalog searches; lending/sessions; currency-aware Money;
and assertion-based testing.

New tests cover blank IDs, combined filters and all sort modes, year boundaries,
no results, currency sorting rejection, borrowers/dates/partial returns, legacy
and named-loan isolation, invalid duration, vendor metadata/NaN/duplicate IDs,
strategies and ties, request lifecycle, cancellation snapshots, config rollback,
shared-budget atomic rejection, holdings overflow, mixed-currency batch rejection,
empty/zero/unlimited summaries, percentages, chronological events, Money limits,
and generator uniqueness/counts. Allocation failures exercise purchase, borrow,
named partial return, cancellation, atomic batch, submit and anonymous return.

## Actual benchmark results

One run per dataset; timings are milliseconds and include result construction.
ID lookup measures 10,000 calls with prebuilt ID strings. Other operations are
one query each. These are unoptimized synthetic timings, not latency guarantees.

| Operation | 10,000 | 100,000 | 1,000,000 |
|---|---:|---:|---:|
| generation | 24.128 | 255.494 | 2970.640 |
| ID lookup (10000 calls) | 5.464 | 12.723 | 14.772 |
| title search | 4.484 | 49.710 | 571.211 |
| author search | 1.729 | 19.287 | 220.334 |
| combined search | 4.421 | 49.091 | 539.769 |
| year range | 1.057 | 13.233 | 141.850 |

| Query matches | 10,000 | 100,000 | 1,000,000 |
|---|---:|---:|---:|
| ID lookup (10000 calls) | 10000 | 10000 | 10000 |
| title search | 2000 | 20000 | 200000 |
| author search | 34 | 334 | 3334 |
| combined search | 241 | 2411 | 24113 |
| year range | 2338 | 23398 | 234036 |

## Compatibility decisions and limits

- Legacy APIs and includes remain. Legacy returns close only anonymous loans;
  use the explicit named-loan API for borrower records.
- Purchase history describes actions. Approved originals remain unchanged;
  cancellation appends a record. `orderStatus` provides current state.
- Approved summary counts include later-cancelled orders; amounts are net after refunds.
- Legacy vendor offers default to rating 0 and delivery days 0. Provide explicit
  metadata when fastest ranking matters; 0 is a real ranking value, not unknown.
- Currency totals reject mixing; there is no implicit conversion. Prices sort
  only within one currency. Quota utilization is optional when unlimited.
- IDs are validated but not trimmed. Search is case-insensitive for text,
  while resource/borrower IDs remain exact and case-sensitive.
- Dates are chrono instants; default duration is elapsed 24-hour days.
- In-memory, single-threaded operation; callers synchronize concurrent access.
  History and loan retention are unbounded. Staging copies are intentionally
  simple and are not optimized for large-history write workloads.
- Order numbers are manager-local. Use a single AcquisitionManager per catalog
  if consumers need unambiguous order references in the shared audit view.
- No Windows execution or LeakSanitizer result is claimed.

## Files and delivery

README.md has architecture, API examples, validation/tie rules, Linux/Windows
commands, testing and benchmark instructions. examples/demo.cpp retains original
demonstrations and adds labeled V1 search/lending/vendor/report sections.
CMake builds library, demo, main tests, fault tests and benchmark. All build
folders/binaries are excluded from the delivery ZIP; build/ and build-*/ are ignored.
ANSWERS.md is retained unchanged as historical assignment material.
