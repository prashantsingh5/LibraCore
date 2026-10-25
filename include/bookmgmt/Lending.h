#pragma once
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
namespace bookmgmt {
using Timestamp = std::chrono::system_clock::time_point;
struct LendingConfig { int defaultLoanDays = 14; };
enum class LoanStatus { Active, Returned };
struct Loan {
    std::string loanId, resourceId, borrowerId;
    int quantity = 0;
    Timestamp borrowDate, dueDate;
    std::optional<Timestamp> returnDate;
    LoanStatus status = LoanStatus::Active;
    int returnedQuantity = 0;
    bool anonymous = false;
    int outstanding() const { return quantity - returnedQuantity; }
};
struct LendingSummary {
    std::int64_t totalHoldings = 0, borrowed = 0, available = 0;
    std::size_t activeLoans = 0;
};
}
