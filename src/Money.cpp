#include "bookmgmt/Money.h"

#include <algorithm>
#include <cctype>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <limits>

namespace bookmgmt {
namespace {
std::int64_t checkedAdd(std::int64_t a, std::int64_t b) {
    const auto hi = std::numeric_limits<std::int64_t>::max();
    const auto lo = std::numeric_limits<std::int64_t>::min();
    if ((b > 0 && a > hi - b) || (b < 0 && a < lo - b))
        throw std::overflow_error("Money addition overflow");
    return a + b;
}
std::int64_t checkedMultiply(std::int64_t a, std::int64_t b) {
    const auto hi = std::numeric_limits<std::int64_t>::max();
    const auto lo = std::numeric_limits<std::int64_t>::min();
    if (a > 0 ? (b > 0 ? a > hi / b : b < lo / a)
              : (a < 0 && (b > 0 ? a < lo / b : b < 0 && a < hi / b)))
        throw std::overflow_error("Money multiplication overflow");
    return a * b;
}
}
Money& Money::operator*=(std::int64_t k) {
    minor_ = checkedMultiply(minor_, k); return *this;
}
Money Money::scaledRatio(int numerator, int denominator) const {
    if (numerator < 0 || denominator <= 0) throw std::invalid_argument("invalid Money ratio");
    return fromMinor(checkedAdd(checkedMultiply(minor_ / denominator, numerator),
                               checkedMultiply(minor_ % denominator, numerator) / denominator), currency_);
}


std::string Money::normalizeCurrency(
    std::string currency) {

    if (currency.empty()) {
        throw std::invalid_argument(
            "currency code must not be empty");
    }

    if (currency.size() != 3) {
        throw std::invalid_argument(
            "currency code must contain exactly 3 letters");
    }

    for (char& ch : currency) {
        const unsigned char uch =
            static_cast<unsigned char>(ch);

        if (!std::isalpha(uch)) {
            throw std::invalid_argument(
                "currency code must contain only letters");
        }

        ch = static_cast<char>(
            std::toupper(uch));
    }

    return currency;
}

Money::Money(
    std::int64_t minor,
    std::string currency)
    : minor_(minor),
      currency_(normalizeCurrency(
          std::move(currency))) {}

Money Money::fromMinor(
    std::int64_t minor,
    std::string currency) {

    return Money(
        minor,
        std::move(currency));
}

Money Money::of(
    std::int64_t major,
    int minor,
    std::string currency) {

    if (minor < 0 || minor > 99) {
        throw std::invalid_argument(
            "minor part must be in 0..99");
    }

    const std::int64_t sign =
        major < 0 ? -1 : 1;

    return Money(
        checkedAdd(checkedMultiply(major, 100), sign * minor),
        std::move(currency));
}

void Money::requireSameCurrency(
    const Money& a,
    const Money& b) {

    if (a.currency_ != b.currency_) {
        throw std::invalid_argument(
            "currency mismatch: " +
            a.currency_ +
            " vs " +
            b.currency_);
    }
}

Money& Money::operator+=(
    const Money& other) {

    requireSameCurrency(*this, other);

    minor_ = checkedAdd(minor_, other.minor_);

    return *this;
}

Money& Money::operator-=(
    const Money& other) {

    requireSameCurrency(*this, other);

    const auto hi = std::numeric_limits<std::int64_t>::max();
    const auto lo = std::numeric_limits<std::int64_t>::min();
    if ((other.minor_ < 0 && minor_ > hi + other.minor_) ||
        (other.minor_ > 0 && minor_ < lo + other.minor_))
        throw std::overflow_error("Money subtraction overflow");
    minor_ -= other.minor_;

    return *this;
}

bool operator==(
    const Money& a,
    const Money& b) {

    Money::requireSameCurrency(a, b);

    return a.minor_ == b.minor_;
}

bool operator!=(
    const Money& a,
    const Money& b) {

    return !(a == b);
}

bool operator<(
    const Money& a,
    const Money& b) {

    Money::requireSameCurrency(a, b);

    return a.minor_ < b.minor_;
}

bool operator<=(
    const Money& a,
    const Money& b) {

    Money::requireSameCurrency(a, b);

    return a.minor_ <= b.minor_;
}

bool operator>(
    const Money& a,
    const Money& b) {

    Money::requireSameCurrency(a, b);

    return a.minor_ > b.minor_;
}

bool operator>=(
    const Money& a,
    const Money& b) {

    Money::requireSameCurrency(a, b);

    return a.minor_ >= b.minor_;
}

std::string Money::toString() const {
    const std::uint64_t absMinor = minor_ < 0
        ? static_cast<std::uint64_t>(-(minor_ + 1)) + 1
        : static_cast<std::uint64_t>(minor_);

    std::string s =
        std::to_string(absMinor / 100);

    const auto frac =
        absMinor % 100;

    s += '.';

    if (frac < 10) {
        s += '0';
    }

    s += std::to_string(frac);

    return minor_ < 0
               ? "-" + s
               : s;
}

std::ostream& operator<<(
    std::ostream& os,
    const Money& m) {

    return os << m.toString();
}

}  // namespace bookmgmt