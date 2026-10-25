#pragma once
#include "bookmgmt/MT26076_AudioBook.h"
#include "bookmgmt/MT26076_Thesis.h"
#include "bookmgmt/bookmgmt.h"
#include "libracore/Repository.h"
namespace libracore {
void validateNumericInputs(const Json &);
std::unique_ptr<bookmgmt::Resource> resource(const Json &);
Json record(const bookmgmt::PurchaseRecord &);
bookmgmt::PurchaseRecord record(const Json &);
Json loan(const bookmgmt::Loan &);
bookmgmt::Loan loan(const Json &);
std::string timestamp(bookmgmt::Timestamp);
bookmgmt::Timestamp timestamp(const std::string &);
Json budgetSummary(const bookmgmt::Budget &);
} // namespace libracore
namespace bookmgmt {
struct PersistenceAccess {
  static void holdings(Catalog &c, const std::string &id, int held, int lent) {
    c.items_.at(id).holdings = held;
    c.items_.at(id).lent = lent;
  }
  static void restoreLoan(Catalog &c, Loan l) {
    c.loans_.emplace(l.loanId, std::move(l));
  }
  static void counters(Catalog &c, AcquisitionManager &a, std::uint64_t l,
                       int o) {
    c.nextLoan_ = l;
    a.nextOrderNo_ = o;
  }
  static void history(AcquisitionManager &a, PurchaseRecord r) {
    a.history_.push_back(std::move(r));
  }
  static void usage(Budget &b, ResourceCategory c, int units, Money cost) {
    b.usage_[c] = {units, cost};
  }
  static void title(Budget &b, ResourceCategory c, const std::string &id,
                    int units) {
    b.purchasedTitles_[c][id] = units;
  }
  static void spent(Budget &b, Money m) { b.spent_ = m; }
  static const auto &titles(const Budget &b) { return b.purchasedTitles_; }
  static std::uint64_t nextLoan(const Catalog &c) { return c.nextLoan_; }
  static int nextOrder(const AcquisitionManager &a) { return a.nextOrderNo_; }
  static const auto &loans(const Catalog &c) { return c.loans_; }
};
} // namespace bookmgmt
