#include "bookmgmt/Resource.h"
#include "bookmgmt/Validation.h"

#include <ostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace bookmgmt {

const char* categoryName(ResourceCategory c) {
    switch (c) {
        case ResourceCategory::Book: return "Book";
        case ResourceCategory::ElectronicResource: return "ElectronicResource";
        case ResourceCategory::Journal: return "Journal";
        case ResourceCategory::EBook: return "EBook";
    }
    return "Unknown";
}

Resource::Resource(std::string id, std::string title, std::string publisher,
                   int year, Money unitPrice)
    : id_(std::move(id)),
      title_(std::move(title)),
      publisher_(std::move(publisher)),
      year_(year),
      unitPrice_(unitPrice) {
    requireId(id_, "resource id");
    if (title_.empty()) throw std::invalid_argument("resource title must not be empty");
    if (unitPrice_.isNegative()) throw std::invalid_argument("price must not be negative");
}

void Resource::setUnitPrice(Money price) {
    if (price.isNegative()) throw std::invalid_argument("price must not be negative");
    for (const auto& v : vendors_) if (price.currency() != v.price.currency())
        throw std::invalid_argument("price currency must match existing vendors");
    unitPrice_ = price;
}

void Resource::addVendor(std::string vendor, Money price) {
    const std::string id = vendor;
    addVendor(VendorOffer{std::move(vendor), price, id, 0, 0});
}
void Resource::addVendor(VendorOffer offer) {
    requireId(offer.vendor, "vendor name"); requireId(offer.vendorId, "vendor id");
    if (!std::isfinite(offer.rating) || offer.rating < 0 || offer.rating > 5)
        throw std::invalid_argument("vendor rating must be finite and in 0..5");
    if (offer.deliveryDays < 0) throw std::invalid_argument("delivery days must be nonnegative");
    if (offer.price.isNegative()) throw std::invalid_argument("vendor price must be nonnegative");
    if (offer.price.currency() != unitPrice_.currency())
        throw std::invalid_argument("vendor currency must match resource currency");
    for (const auto& v : vendors_) if (v.vendorId == offer.vendorId)
        throw std::invalid_argument("duplicate vendor id");
    vendors_.push_back(std::move(offer));
}
const VendorOffer* Resource::cheapestVendor() const { return selectVendor(VendorSelection::Cheapest); }
const VendorOffer* Resource::selectVendor(VendorSelection mode) const {
    if (mode != VendorSelection::Cheapest && mode != VendorSelection::Fastest && mode != VendorSelection::HighestRated)
        throw std::invalid_argument("invalid vendor selection mode");
    if (vendors_.empty()) return nullptr;
    auto it = std::min_element(vendors_.begin(), vendors_.end(), [mode](const VendorOffer& a, const VendorOffer& b) {
        if (mode == VendorSelection::Fastest && a.deliveryDays != b.deliveryDays) return a.deliveryDays < b.deliveryDays;
        if (mode == VendorSelection::HighestRated && a.rating != b.rating) return a.rating > b.rating;
        if (a.price != b.price) return a.price < b.price;
        if (a.rating != b.rating) return a.rating > b.rating;
        if (a.deliveryDays != b.deliveryDays) return a.deliveryDays < b.deliveryDays;
        return a.vendorId < b.vendorId;
    });
    return &*it;
}

void Resource::requirePositive(int quantity) {
    if (quantity <= 0) throw std::invalid_argument("quantity must be positive");
}

Money Resource::costFor(int quantity) const {
    return costForAtPrice(quantity, unitPrice_);
}

Money Resource::costForAtPrice(int quantity, Money unitPrice) const {
    requirePositive(quantity);

    if (unitPrice.isNegative()) {
        throw std::invalid_argument("price must not be negative");
    }

    Money total = unitPrice * quantity;

    // Q5: Print items receive a 10% bulk discount for 10 or more copies.
    if (quantity >= 10) {
        total -= total.scaledRatio(10, 100);
    }

    return total;
}

void Resource::print(std::ostream& os) const {
    os << categoryName(category()) << " " << id_ << "\n"
       << "  title: " << title_ << "\n"
       << "  publisher: " << publisher_ << "\n"
       << "  year: " << year_ << "\n"
       << "  unit price: " << unitPrice_ << "\n";
    printDetails(os);
}

void Resource::printDetails(std::ostream&) const {}

std::string Resource::summary() const {
    std::ostringstream os;
    os << "[" << categoryName(category()) << "] " << id_ << "  " << title_
       << " (" << year_ << ")  @ " << unitPrice_;
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const Resource& r) {
    r.print(os);
    return os;
}

}  // namespace bookmgmt
