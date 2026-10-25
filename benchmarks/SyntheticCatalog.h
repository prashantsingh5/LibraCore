#pragma once
#include "bookmgmt/bookmgmt.h"
#include <iomanip>
#include <sstream>
namespace bookmgmt::benchmark {
inline std::string syntheticId(std::size_t i) {
    std::ostringstream out; out << "SYN-" << std::setw(7) << std::setfill('0') << i;
    return out.str();
}
inline Catalog generateCatalog(std::size_t count) {
    Catalog catalog;
    for (std::size_t i = 0; i < count; ++i) {
        const auto id = syntheticId(i);
        const auto title = std::string(i % 5 == 0 ? "Machine Learning " : "Software Engineering ") + std::to_string(i);
        const auto price = Money::of(100 + static_cast<int>(i % 1900));
        const int year = 1980 + static_cast<int>(i % 47);
        if (i % 3 == 0)
            catalog.emplace<Book>(id, title, std::vector<std::string>{"Author " + std::to_string(i%100)},
                                  "ISBN-" + std::to_string(i), "Academic Press", year, price);
        else if (i % 3 == 1)
            catalog.emplace<Journal>(id, title, "ISSN-" + std::to_string(i), 12, "Research Press", year, price);
        else
            catalog.emplace<ElectronicResource>(id, title, "Digital Press", year, price,
                                                "https://example.test/resources/" + id);
    }
    return catalog;
}
}
