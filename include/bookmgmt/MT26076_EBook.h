// Name: Prashant Singh
// Roll No: MT26076

#pragma once

#include <string>
#include <vector>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

enum class EBookFormat {
    PDF,
    EPUB,
    HTML
};

const char* ebookFormatName(EBookFormat format);

// Design Note:
// Book and Ebook both has some common information 
// ie, authors, ISBN that we are adding in both Book nad EBook 
// classes, This is duplication. So to resolve this we could add a 
// parent calss that contain this information once, then they can 
// just inherit the parent class. 

class EBook : public ElectronicResource {
public:
    EBook(std::string id,
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
          Money platformFee = Money::of(0));

    const std::vector<std::string>& authors() const { return authors_; }
    const std::string& isbn() const { return isbn_; }
    EBookFormat format() const { return format_; }
    bool drmProtected() const { return drmProtected_; }

    ResourceCategory category() const override {
        return ResourceCategory::EBook;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::vector<std::string> authors_;
    std::string isbn_;
    EBookFormat format_;
    bool drmProtected_;
};

}  // namespace bookmgmt