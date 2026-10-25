#include "bookmgmt/ElectronicResource.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

const char* licenseName(LicenseModel m) {
    switch (m) {
        case LicenseModel::Perpetual: return "Perpetual";
        case LicenseModel::AnnualSubscription: return "Annual subscription";
    }
    return "Unknown";
}

ElectronicResource::ElectronicResource(std::string id, std::string title,
                                       std::string publisher, int year,
                                       Money pricePerSeat, std::string accessUrl,
                                       LicenseModel license, Money platformFee)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, pricePerSeat),
      accessUrl_(std::move(accessUrl)),
      license_(license),
      platformFee_(platformFee) {
    if (platformFee_.isNegative())
        throw std::invalid_argument("platform fee must not be negative");
}

Money ElectronicResource::costFor(int seats) const {
    return costForAtPrice(seats, unitPrice());
}

Money ElectronicResource::costForAtPrice(
    int seats,
    Money pricePerSeat) const {

    requirePositive(seats);

    if (pricePerSeat.isNegative()) {
        throw std::invalid_argument("price must not be negative");
    }

    if (seats <= 50) {
        return platformFee_ +
               pricePerSeat * seats;
    }

    const int fullPriceSeats = 50;
    const int halfPriceSeats = seats - 50;

    Money fullPriceCost =
        pricePerSeat * fullPriceSeats;

    Money halfPriceCost =
        pricePerSeat.scaledRatio(halfPriceSeats, 2);

    return platformFee_ +
           fullPriceCost +
           halfPriceCost;
}

void ElectronicResource::printDetails(std::ostream& os) const {
    os << "  access url: " << accessUrl_ << "\n"
       << "  license: " << licenseName(license_) << "\n"
       << "  platform fee: " << platformFee_ << "\n";
}

}  // namespace bookmgmt
