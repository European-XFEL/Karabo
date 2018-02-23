/*
 * File:   Overflow_Test.hh
 * Author: gero.flucke@xfel.eu
 *
 * Created on Feb 23, 2018, 2:35 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Overflow_Test.hh"
#include "karabo/util/Overflow.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Overflow_Test);


Overflow_Test::Overflow_Test() {
}


Overflow_Test::~Overflow_Test() {
}


void Overflow_Test::setUp() {
}


void Overflow_Test::tearDown() {
}


void Overflow_Test::testOverflow() {

    using karabo::util::safeAddToFirst;
    // No overflow
    unsigned long long a = 1234567ull;
    CPPUNIT_ASSERT_EQUAL(0ull, safeAddToFirst(a, 20ull));
    CPPUNIT_ASSERT_EQUAL(1234587ull, a);

    a = (1ull << 63) + 3ull;
    CPPUNIT_ASSERT_EQUAL(1ull, safeAddToFirst(a, (1ull << 63)));
    CPPUNIT_ASSERT_EQUAL(3ull, a);

    using karabo::util::safeMultiply;
    // simple case - no overflow
    const auto big128_a = safeMultiply(4ull, 100000ull);
    CPPUNIT_ASSERT_EQUAL(0ull, big128_a.first);
    CPPUNIT_ASSERT_EQUAL(400000ull, big128_a.second);

    // now with an overflow
    const unsigned long long maxULL = -1;
    const auto big128_b = safeMultiply(4ull, maxULL);
    CPPUNIT_ASSERT_EQUAL(3ull, big128_b.first);
    CPPUNIT_ASSERT_EQUAL(maxULL - 3ull, big128_b.second);
    const auto big128_c = safeMultiply(1000ull, maxULL - 3000ull);
    CPPUNIT_ASSERT_EQUAL(999ull, big128_c.first);
    CPPUNIT_ASSERT_EQUAL(maxULL - 3001000ull + 1ull, big128_c.second);

    // now an overflow where the internal safeAdd has an overflow
    const unsigned long long lower33bits = (1ull << 33ull) - 1ull;
    const auto big128_d = safeMultiply(lower33bits, maxULL);
    CPPUNIT_ASSERT_EQUAL(lower33bits - 1ull, big128_d.first);
    CPPUNIT_ASSERT_EQUAL(maxULL - lower33bits + 1ull, big128_d.second);
}
