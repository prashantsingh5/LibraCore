#include "TestSupport.h"
void testV1Search() {
    Catalog c;
    c.emplace<Book>("B1", "Zulu", std::vector<std::string>{"Martin"}, "isbn", "P", 2000, Money::of(30));
    c.emplace<Book>("B2", "Alpha", std::vector<std::string>{"Martin"}, "isbn2", "P", 2020, Money::of(10));
    c.emplace<Journal>("J", "Middle", "issn", 12, "P", 2010, Money::of(20));
    assertThrows<std::invalid_argument>([&] { c.emplace<Book>(" \t", "T", std::vector<std::string>{}, "", "P", 2000, Money{}); });
    SearchQuery q; q.author = "MART"; q.category = ResourceCategory::Book; q.fromYear = 2000; q.toYear = 2000;
    assert(c.search(q).size() == 1 && c.search(q)[0]->id() == "B1");
    q.title = "nope"; assert(c.search(q).empty());
    q = {}; q.identifier = "ISSN"; assert(c.search(q).at(0)->id() == "J");
    q = {}; q.category = ResourceCategory::Journal; q.fromYear = 2010; assert(c.search(q).size() == 1);
    q = {}; q.fromYear = 2021; q.toYear = 2000;
    assertThrows<std::invalid_argument>([&] { c.search(q); });
    for (auto sort : {SearchSort::TitleAscending, SearchSort::PriceAscending, SearchSort::YearDescending}) {
        q = {}; q.sort = sort; assert(c.search(q).front()->id() == "B2");
    }
    for (auto sort : {SearchSort::TitleDescending, SearchSort::PriceDescending, SearchSort::YearAscending}) {
        q = {}; q.sort = sort; assert(c.search(q).front()->id() == "B1");
    }
    c.emplace<Book>("B0", "Alpha", std::vector<std::string>{}, "", "", 2020, Money::of(10));
    q.sort = SearchSort::PriceAscending; assert(c.search(q).front()->id() == "B0");
    c.emplace<Book>("USD", "USD", std::vector<std::string>{}, "", "", 2020, Money::of(10,0,"USD"));
    assertThrows<std::invalid_argument>([&] { c.search(q); });
}
