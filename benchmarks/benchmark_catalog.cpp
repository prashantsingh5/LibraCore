#include "SyntheticCatalog.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace bookmgmt;
template<class Operation>
void measure(const char* label, Operation operation) {
    const auto start = std::chrono::steady_clock::now();
    const std::size_t matches = operation();
    const double ms = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::cout << label << "," << std::fixed << std::setprecision(3) << ms << "," << matches << "\n";
}
int main(int argc, char** argv) {
    try {
        if (argc > 2) throw std::invalid_argument("usage: bookmgmt_benchmark [1..1000000]");
        std::size_t count = 10000;
        if (argc == 2) {
            const std::string arg = argv[1]; std::size_t used = 0;
            if (arg.empty() || arg.find_first_not_of("0123456789") != std::string::npos)
                throw std::invalid_argument("dataset size must be a positive integer");
            count = std::stoull(arg, &used);
            if (used != arg.size() || count == 0 || count > 1000000)
                throw std::invalid_argument("dataset size must be 1..1000000");
        }
        const auto start = std::chrono::steady_clock::now();
        auto c = benchmark::generateCatalog(count);
        const auto generated = std::chrono::steady_clock::now();
        std::cout << "Dataset: " << c.size() << " resources\n";
        std::cout << "operation,elapsed_ms,matches\n";
        std::cout << "generation," << std::chrono::duration<double,std::milli>(generated-start).count() << "," << c.size() << "\n";
        std::vector<std::string> ids;
        for (std::size_t i=0; i<10000; ++i) ids.push_back(benchmark::syntheticId((i*997)%count));
        measure("ID lookup (10000 calls)",[&] { std::size_t hits=0; for (const auto& id:ids) hits += c.find(id) != nullptr; return hits; });
        measure("title search",[&] { return c.searchTitle("Machine Learning").size(); });
        measure("author search",[&] { return c.searchAuthor("Author 42").size(); });
        SearchQuery q; q.category=ResourceCategory::Book; q.title="Machine"; q.fromYear=2010; q.toYear=2026;
        measure("combined search",[&] { return c.search(q).size(); });
        measure("year range",[&] { return c.searchYearRange(2010,2020).size(); });
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << "\n"; return 1; }
}
