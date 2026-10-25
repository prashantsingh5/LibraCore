#include <iostream>
void testMoney();
void testResourcesAndCost();
void testJournal();
void testEBook();
void testAudioBookAndThesis();
void testCatalog();
void testBudget();
void testAcquisition();
void testTaxes();
void testTitleQuota();
void testCancellation();
void testDepartments();
void testYearEndRollover();
void testAtomicBatch();
void testVendors();
void testCatalogSearches();
void testLending();
void testCurrencyAwareMoney();
void testV1Search();
void testV1Lending();
void testV1Vendors();
void testV1Acquisition();
void testV1Transactions();
void testV1Reports();
void testV1Money();
void testGenerator();
int main() {
    testMoney();
    testResourcesAndCost();
    testJournal();
    testEBook();
    testAudioBookAndThesis();
    testCatalog();
    testBudget();
    testAcquisition();
    testTaxes();
    testTitleQuota();
    testCancellation();
    testDepartments();
    testYearEndRollover();
    testAtomicBatch();
    testVendors();
    testCatalogSearches();
    testLending();
    testCurrencyAwareMoney();

    testV1Search();
    testV1Lending();
    testV1Vendors();
    testV1Acquisition();
    testV1Transactions();
    testV1Reports();
    testV1Money();
    testGenerator();
    std::cout << "All tests passed.\n";
    return 0;
}
    