# LibraCore V1.0 — final stabilization and release freeze

24 October 2026. Scope: verify, polish, document and package the existing C++17
engine. No new domain features, frameworks, integrations, commits, tags or pushes.

## A. Baseline

Before source modifications, configured and built a fresh build directory.
All 26 main test functions, seven fault-injection scenarios (58 allocation-failure
points), the complete demo and 10K/100K/1M benchmarks passed. Baseline compilation
had no warnings. CMake was restored in the execution environment; it is a build
tool, not a project dependency added to the C++ engine. Exact baseline logs are
under `docs/release-v1.0/baseline-*.txt`.

## B. Changes made

| Existing file | Change | Reason |
|---|---|---|
| .gitignore | Added CMake-generated files, editor backups and core-dump rules. | Keep generated files out of source control. |
| README.md | Added portfolio title/version, feature summary, academic-name explanation, actual hierarchy, test/demo notes, fresh benchmarks/environment, limitations, roadmap and manual tagging instructions. | Make the release understandable and reproducible. |
| benchmarks/benchmark_catalog.cpp | Capture the generation end timestamp before writing output. | Exclude console output from generation timing. |
| examples/demo.cpp | Show two sorted matches, highest-rated quote and four actual event types. | Demonstrate existing behavior clearly without adding domain features. |
| include/bookmgmt/Acquisition.h | Replace stale extension-point comment with allocation-failure semantics. | Describe the implemented batch API accurately. |
| include/bookmgmt/Resource.h | Correct hierarchy and base pricing comments. | Match actual inheritance and existing bulk-discount behavior. |

All `src/*.cpp` domain implementations, all test files, CMake targets, academic
filenames, include paths and `ANSWERS.md` remain byte-for-byte unchanged from
the pre-polish source snapshot. No new public API was necessary.

New documentation/evidence files:

| File | Purpose |
|---|---|
| docs/RELEASE_FREEZE_REPORT.md | This release review and verdict |
| docs/release-v1.0/baseline-benchmark-10000.txt | Recorded untouched baseline run |
| docs/release-v1.0/baseline-benchmark-100000.txt | Recorded untouched baseline run |
| docs/release-v1.0/baseline-benchmark-1000000.txt | Recorded untouched baseline run |
| docs/release-v1.0/baseline-build.txt | Recorded untouched baseline run |
| docs/release-v1.0/baseline-configure.txt | Recorded untouched baseline run |
| docs/release-v1.0/baseline-demo.txt | Recorded untouched baseline run |
| docs/release-v1.0/baseline-fault-tests.txt | Recorded untouched baseline run |
| docs/release-v1.0/baseline-main-tests.txt | Recorded untouched baseline run |
| docs/release-v1.0/benchmark-review.txt | Fairness and invalid CLI argument checks |
| docs/release-v1.0/demo-review.txt | Original output preservation and V1 demonstration checks |
| docs/release-v1.0/environment.txt | Compiler, OS, CPU and build-mode details |
| docs/release-v1.0/final-benchmark-10000.txt | Recorded final clean-build verification |
| docs/release-v1.0/final-benchmark-100000.txt | Recorded final clean-build verification |
| docs/release-v1.0/final-benchmark-1000000.txt | Recorded final clean-build verification |
| docs/release-v1.0/final-build.txt | Recorded final clean-build verification |
| docs/release-v1.0/final-configure.txt | Recorded final clean-build verification |
| docs/release-v1.0/final-ctest.txt | Recorded final clean-build verification |
| docs/release-v1.0/final-demo.txt | Recorded final clean-build verification |
| docs/release-v1.0/final-fault-tests.txt | Recorded final clean-build verification |
| docs/release-v1.0/final-main-tests.txt | Recorded final clean-build verification |
| docs/release-v1.0/repository-review.txt | Git availability, source/test preservation and cleanliness review |

## C. README improvements

Portfolio identity and version; complete feature overview; academic compatibility;
accurate architecture/direct-parent hierarchy; test and fault-test explanation;
demo coverage; measured benchmark results and environment; structured data APIs;
Current V1 Limitations; short V1/V2/V3 roadmap; manual tag instructions.

## D. Actual benchmark results

Milliseconds, single run per dataset after final clean build:

| Dataset | Generation | 10K ID lookups | Title search | Author search | Combined search | Year range |
|---:|---:|---:|---:|---:|---:|---:|
| 10,000 | 24.709 | 6.125 | 4.750 | 1.858 | 4.807 | 1.062 |
| 100,000 | 268.512 | 12.339 | 54.594 | 22.209 | 51.026 | 13.914 |
| 1,000,000 | 2834.960 | 15.440 | 531.468 | 197.178 | 518.135 | 128.449 |

GCC 13.3.0; CMake 4.4.4; Linux 6.18.55 x86_64; AMD EPYC 9V74 80-Core Processor (virtualized environment). CMake build type unset; no optimization flag added.

Benchmark results are environment-specific and are intended for comparative
project evaluation rather than universal performance claims. Each lookup timing
covers 10,000 all-hit calls with prebuilt ID strings; other operations are one
query. Counts are consumed and printed. Data generation is timed separately,
now excluding output. Requested sizes were created; existing generator tests
verify uniqueness/count, and duplicate resource IDs are rejected by Catalog.
Raw logs also retain match counts. No benchmark redesign was needed.

## E. Tests and API review

- Main tests: **26 functions passed**, including all 18 original cases.
- Fault tests: **7 scenarios, 58 allocation-failure points passed**.
- CTest: **2/2 targets passed**.
- No tests removed, rewritten, disabled or newly added in this stabilization pass.

Coverage reviewed: Money/currencies, all resource types, hardcover/bulk/electronic
pricing, taxes, budget/unit/spend/title quotas, departments, rollover, refunds,
cancellation, independent/atomic batches, vendor modes, search/filters/sorts,
lending/loan histories/due dates, reporting/utilization, events and exception safety.
Fault scenarios include purchase, cancel, borrow, named/anonymous return, atomic
batch and submit, checking no partial mutation at each injected allocation failure.

Money remains signed 64-bit fixed-point, preserves currency and uppercase
normalization, rejects invalid codes and mixed arithmetic/comparisons, and keeps
both legacy numeric and explicit currency display; existing tests passed.

Public structured access is already present: BudgetSummary, CategorySummary,
DepartmentSummary, AcquisitionSummary, LendingSummary, SearchQuery, Loan,
VendorOffer, PurchaseRecord and Event. Future external code need not parse
console output. No serialization or external integration was introduced.

## F. Build

Fresh configure **passed**. Full build **passed**. **Zero compiler warnings**
with existing `-Wall -Wextra -Wpedantic`. No warning suppression added. Assertions
remain enabled by existing test configuration. Build artifacts are excluded
from delivery. Windows instructions are retained but were not executed here.
The earlier sanitizer results remain historical; this polish pass ran the
requested clean builds/tests rather than repeating unrelated sanitizer checks.

## G. Demo

Complete demo exited successfully. Original output before V1 sections matched
the baseline exactly. V1 visibly demonstrates two newest-first search results,
named borrower and 14-day due period, return, all three vendor modes, structured
budget/department/lending/acquisition reports, currency display and four audit
event types. Only a few additional output lines were added.

## H. Repository cleanliness

Input is a source ZIP with **no .git metadata**. `git status`, `git diff --stat`,
`git diff` and `git ls-files` were attempted and cannot report repository state.
No repository was initialized. A full file-by-file diff against the pre-polish
snapshot was reviewed instead; six existing files changed as listed above.
Tracked-file cleanliness in the user's actual Git checkout cannot be asserted.

No backup/editor/core-dump files or stray source-tree binaries were found.
Old build directories and intermediate polish output were moved outside the
source tree, preserving them only as scratch evidence. Final build/ is ignored
and excluded from the release ZIP. Historical documentation and verification
logs were retained deliberately. The release archive contains no build folders,
binaries, backup files or generated benchmark datasets.

## I. Intentional V1 limitations

In-memory, single-process, single-threaded, C++ only and non-persistent; no
authentication, database or AI. History retention is unbounded. Transaction
staging favors exception safety over high-volume write throughput. Benchmarks
cover catalog search, not large-history transaction performance. Persistence
and Python/AI belong to later versions.

## J. Manual release and verdict

After placing and committing these reviewed files in your actual Git checkout,
run manually if desired:

```sh
git tag -a v1.0.0 -m "LibraCore V1 C++ core"
git push origin v1.0.0
```

No commits, tags or pushes were performed. No blocking V1 issue remains in the
verified source release.

V1 READY TO FREEZE
