// Name: Prashant Singh
// Roll No: MT26076

#pragma once

#include <string>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

// Design choice: AudioBook is getting derived from ElectronicResource because
// audiobook is a digitallibrary. which allow us to reuse the electronic access,
// licensing and pricing behavior.so we can only add sudiobook-specific information
// like narrator and duration.

class AudioBook : public ElectronicResource {
public:
    AudioBook(std::string id,
              std::string title,
              std::string narrator,
              int durationMinutes,
              std::string publisher,
              int year,
              Money unitPrice,
              std::string accessUrl,
              LicenseModel licenseModel,
              Money platformFee = Money::of(0));

    const std::string& narrator() const { return narrator_; }
    int durationMinutes() const { return durationMinutes_; }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string narrator_;
    int durationMinutes_;
};

}  // namespace bookmgmt