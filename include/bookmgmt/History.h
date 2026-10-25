#pragma once
#include "bookmgmt/Lending.h"
namespace bookmgmt {
enum class EventType { Purchase, Cancellation, Borrow, Return };
struct Event {
    EventType type;
    std::string resourceId;
    int quantity;
    std::string actor; // borrower or department; empty for default budget
    Timestamp timestamp;
    std::string referenceId; // loan ID or order number
};
}
