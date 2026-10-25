// Name: Prashant Singh
// Roll No: MT26076

#pragma once

#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

// Design choice: This class Thesis is derived from the Resources because
// a thesis is library resourc but it dont necessarily have ISBN, edition and
// all that the convevtional Book has. It can also has price = 0 when it is free.

class Thesis : public Resource {
public:
    Thesis(std::string id,
           std::string title,
           std::string university,
           std::string degree,
           std::string supervisor,
           std::string publisher,
           int year,
           Money unitPrice = Money::of(0));

    const std::string& university() const { return university_; }
    const std::string& degree() const { return degree_; }
    const std::string& supervisor() const { return supervisor_; }

    ResourceCategory category() const override {
        return ResourceCategory::Book;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string university_;
    std::string degree_;
    std::string supervisor_;
};

}  // namespace bookmgmt