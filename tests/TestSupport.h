#pragma once
// Minimal self-contained test runner (no external framework needed).
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

template <typename Exception, typename Func>
void assertThrows(Func&& func) {
    bool correctExceptionThrown = false;

    try {
        func();
    } catch (const Exception&) {
        correctExceptionThrown = true;
    } catch (...) {
        // A different exception type does not satisfy the test.
    }

    assert(correctExceptionThrown);
}

