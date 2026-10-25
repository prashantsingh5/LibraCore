#include "bookmgmt/Book.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

std::string joinAuthors(const std::vector<std::string>& authors) {
    std::string out;
    for (std::size_t i = 0; i < authors.size(); ++i) {
        if (i > 0) out += (i + 1 == authors.size()) ? " and " : ", ";
        out += authors[i];
    }
    return out;
}

Book::Book(std::string id, std::string title, std::vector<std::string> authors,
           std::string isbn, std::string publisher, int year, Money unitPrice,
           int edition, Binding binding)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      edition_(edition),
      binding_(binding) {
    if (edition_ < 1) throw std::invalid_argument("edition must be >= 1");
}

void Book::printDetails(std::ostream& os) const {
    os << "  authors: " << joinAuthors(authors_) << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  edition: " << edition_ << "\n"
       << "  binding: " << (binding_ == Binding::Hardcover ? "Hardcover" : "Paperback")
       << "\n";
}

Money Book::costFor(int quantity) const {
    return costForAtPrice(quantity, unitPrice());
}

Money Book::costForAtPrice(int quantity, Money price) const {
    requirePositive(quantity);

    if (price.isNegative()) {
        throw std::invalid_argument("price must not be negative");
    }

    Money total = price * quantity;

    // Q4: Hardcover books cost 20% more.
    if (binding_ == Binding::Hardcover) {
        total += total.scaledRatio(20, 100);
    }

    // Q5: Print items receive a 10% bulk discount for
    // 10 or more copies.
    if (quantity >= 10) {
        total -= total.scaledRatio(10, 100);
    }

    return total;
}

}  // namespace bookmgmt
