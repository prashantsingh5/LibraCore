# Final polish verification evidence

`baseline-native-exact.log` and `baseline-python-exact.log` come from an untouched isolated extraction of the uploaded ZIP; `baseline-frontend.log` records its original 26 tests. `final-*` logs record the polished tree after corrections. Native build uses C++17 with project warnings; configuration uses extracted libpq/libpqxx headers/libraries because no server is available. No private environment or provider keys were used.

The successful native run was a clean rebuild after interrupted earlier scratch runs left an empty object. Browser review found and fixed a low-contrast disclosure before the final 12/12 run. Only the definitive logs are packaged; intermediate attempts are not test-pass evidence.

`e2e-discovery.log` lists 14 real full-stack journeys but does not claim execution. 48 database tests explicitly skip. Docker builds/Compose, migration execution, real E2E and live Gemini were not run. See `../V3_POLISH_REPORT.md` for commands and boundaries. Browser fixtures exercise responsive screens and real Chromium/axe without PostgreSQL.
