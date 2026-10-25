// Name: Prashant Singh
// Roll No: MT26076

#include "bookmgmt/MT26076_EBook.h"

#include <ostream>
#include <utility>

namespace bookmgmt {

const char* ebookFormatName(EBookFormat format) {
    switch (format) {
        case EBookFormat::PDF:
            return "PDF";
        case EBookFormat::EPUB:
            return "EPUB";
        case EBookFormat::HTML:
            return "HTML";
    }
    return "Unknown";
}

EBook::EBook(std::string id,
             std::string title,
             std::vector<std::string> authors,
             std::string isbn,
             EBookFormat format,
             bool drmProtected,
             std::string publisher,
             int year,
             Money unitPrice,
             std::string accessUrl,
             LicenseModel licenseModel,
             Money platformFee)
    : ElectronicResource(std::move(id), std::move(title),
                         std::move(publisher), year, unitPrice,
                         std::move(accessUrl), licenseModel, platformFee),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      format_(format),
      drmProtected_(drmProtected) {}

void EBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);

    os << "  authors: ";
    for (std::size_t i = 0; i < authors_.size(); ++i) {
        if (i > 0) os << ", ";
        os << authors_[i];
    }

    os << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  format: " << ebookFormatName(format_) << "\n"
       << "  drm protected: " << (drmProtected_ ? "yes" : "no") << "\n";
}

}  // namespace bookmgmt