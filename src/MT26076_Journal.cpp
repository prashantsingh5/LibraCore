// Name: Prashant Singh
// Roll No: MT26076

#include "bookmgmt/MT26076_Journal.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

Journal::Journal(std::string id, std::string title, std::string issn,
                 int issuesPerYear, std::string publisher, int year,
                 Money unitPrice, int subscriptionYears)
    : Resource(std::move(id), std::move(title), std::move(publisher),
               year, unitPrice),
      issn_(std::move(issn)),
      issuesPerYear_(issuesPerYear),
      subscriptionYears_(subscriptionYears) {
    if (subscriptionYears_ < 1) {
        throw std::invalid_argument(
            "subscription years must be at least 1");
    }
}

Money Journal::costFor(int copies) const {
    return costForAtPrice(copies, unitPrice());
}

Money Journal::costForAtPrice(int copies, Money price) const {
    requirePositive(copies);

    if (price.isNegative()) {
        throw std::invalid_argument("price must not be negative");
    }

    Money total =
        price * copies * subscriptionYears_;

    // Q5: Journals are print items, so 10 or more copies receive 10% off.
    if (copies >= 10) {
        total -= total.scaledRatio(10, 100);
    }

    return total;
}
 
void Journal::printDetails(std::ostream& os) const {
    os << "  issn: " << issn_ << "\n"
       << "  issues per year: " << issuesPerYear_ << "\n"
       << "  subscription years: " << subscriptionYears_ << "\n";
}

}  // namespace bookmgmt