#pragma once
// Money: fixed-point currency amount stored in minor units
// (e.g. paise/cents).
//
// Q15: Every Money value also carries a currency code.
// Arithmetic and comparisons between different currencies are rejected.

#include <cstdint>
#include <iosfwd>
#include <string>

namespace bookmgmt {

class Money {
public:
    // Existing code that constructs Money{} continues to use INR.
    Money() = default;

    // Construct from minor units:
    // Money::fromMinor(12550) == INR 125.50
    static Money fromMinor(
        std::int64_t minor,
        std::string currency = "INR");

    // Construct from major + minor parts:
    // Money::of(125, 50) == INR 125.50
    static Money of(
        std::int64_t major,
        int minor = 0,
        std::string currency = "INR");

    std::int64_t minorUnits() const {
        return minor_;
    }

    double toDouble() const {
        return static_cast<double>(minor_) / 100.0;
    }

    // Keeps the old numeric representation so all existing
    // reports/tests remain compatible.
    std::string toString() const;
    std::string toCurrencyString() const { return currency_ + " " + toString(); }

    const std::string& currency() const {
        return currency_;
    }

    bool isZero() const {
        return minor_ == 0;
    }

    bool isNegative() const {
        return minor_ < 0;
    }

    Money& operator+=(const Money& other);
    Money& operator-=(const Money& other);

    Money& operator*=(std::int64_t k);
    Money scaledRatio(int numerator, int denominator) const;

    friend Money operator+(Money a, const Money& b) {
        return a += b;
    }

    friend Money operator-(Money a, const Money& b) {
        return a -= b;
    }

    friend Money operator*(Money a, std::int64_t k) {
        return a *= k;
    }

    friend Money operator*(std::int64_t k, Money a) {
        return a *= k;
    }

    friend bool operator==(const Money& a, const Money& b);
    friend bool operator!=(const Money& a, const Money& b);

    friend bool operator<(const Money& a, const Money& b);
    friend bool operator<=(const Money& a, const Money& b);
    friend bool operator>(const Money& a, const Money& b);
    friend bool operator>=(const Money& a, const Money& b);

private:
    Money(std::int64_t minor, std::string currency);

    static std::string normalizeCurrency(std::string currency);

    static void requireSameCurrency(
        const Money& a,
        const Money& b);

    std::int64_t minor_ = 0;
    std::string currency_ = "INR";
};

std::ostream& operator<<(std::ostream& os, const Money& m);

}  // namespace bookmgmt