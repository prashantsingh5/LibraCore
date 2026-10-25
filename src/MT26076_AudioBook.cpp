// Name: Prashant Singh
// Roll No: MT26076

#include "bookmgmt/MT26076_AudioBook.h"

#include <ostream>
#include <utility>

namespace bookmgmt {

AudioBook::AudioBook(std::string id,
                     std::string title,
                     std::string narrator,
                     int durationMinutes,
                     std::string publisher,
                     int year,
                     Money unitPrice,
                     std::string accessUrl,
                     LicenseModel licenseModel,
                     Money platformFee)
    : ElectronicResource(std::move(id), std::move(title),
                         std::move(publisher), year, unitPrice,
                         std::move(accessUrl), licenseModel, platformFee),
      narrator_(std::move(narrator)),
      durationMinutes_(durationMinutes) {}

void AudioBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);
    os << "  narrator: " << narrator_ << "\n"
       << "  duration: " << durationMinutes_ << " minutes\n";
}

}  // namespace bookmgmt