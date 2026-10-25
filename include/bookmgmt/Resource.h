#pragma once
// Resource: abstract base class for every item the library can hold or buy.
//
// Direct subclasses: Book, Journal, Thesis and ElectronicResource.
// EBook and AudioBook derive from ElectronicResource.

#include <iosfwd>
#include <string>
#include <vector>

#include "bookmgmt/Money.h"

namespace bookmgmt {

// EXTENSION POINT: when adding a new resource type, add a category here,
// a matching name in categoryName() (Resource.cpp) and an entry in
// kAllCategories (Budget.cpp).
enum class ResourceCategory { Book, ElectronicResource, Journal, EBook };

const char* categoryName(ResourceCategory c);

enum class VendorSelection { Cheapest, Fastest, HighestRated };

struct VendorOffer {
    std::string vendor;
    Money price;
    std::string vendorId = {};
    double rating = 0;
    int deliveryDays = 0;
};

class Resource {
public:
    Resource(std::string id, std::string title, std::string publisher,
             int year, Money unitPrice);
    virtual ~Resource() = default;

    // Resources are owned polymorphically (std::unique_ptr); no slicing copies.
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    const std::string& id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& publisher() const { return publisher_; }
    int year() const { return year_; }
    Money unitPrice() const { return unitPrice_; }
    void setUnitPrice(Money price);

    void addVendor(std::string vendor, Money price);
    void addVendor(VendorOffer offer);
    const VendorOffer* selectVendor(VendorSelection mode) const;
    const std::vector<VendorOffer>& vendors() const { return vendors_; }
    const VendorOffer* cheapestVendor() const;

    virtual ResourceCategory category() const = 0;
    virtual bool isDigital() const { return false; }

    // Total cost of acquiring `quantity` units of this resource.
    // "Unit" means copies for print items and user seats for electronic
    // licences. Base pricing applies the print bulk discount at 10+ units.
    // Throws if quantity <= 0.
    // EXTENSION POINT: override for discounts, fees, tiered pricing, ...
    virtual Money costFor(int quantity) const;

    // Calculates the cost using a supplied unit price while preserving
    // the resource type's normal pricing rules.
    virtual Money costForAtPrice(int quantity, Money unitPrice) const;

    // Writes a multi-line human-readable description.
    void print(std::ostream& os) const;
    // Single-line summary, e.g. "[Book] B001  Clean Code (2008)  @ 450.00".
    std::string summary() const;

protected:
    // Subclasses append their own fields (one "  key: value" line each).
    virtual void printDetails(std::ostream& os) const;
    static void requirePositive(int quantity);

private:
    std::string id_;
    std::string title_;
    std::string publisher_;
    int year_;
    Money unitPrice_;
    std::vector<VendorOffer> vendors_;
};

std::ostream& operator<<(std::ostream& os, const Resource& r);

}  // namespace bookmgmt
