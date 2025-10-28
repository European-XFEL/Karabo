/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   ByteSwap_Test.hh
 * Author: parenti
 *
 * Created on October 4, 2013, 3:51 PM
 */

#include <gtest/gtest.h>

#include "karabo/data/types/ByteSwap.hh"

using namespace karabo::data;
using namespace std;

TEST(TestByteSwap, test16) {
    EXPECT_EQ(byteSwap16(0x1234), (uint16_t)0x3412);
}


TEST(TestByteSwap, test32) {
    EXPECT_EQ(byteSwap32(0x12345678), (uint32_t)0x78563412);
}


TEST(TestByteSwap, test64) {
    EXPECT_EQ(byteSwap64(0x1234567890ABCDEF), (uint64_t)0xEFCDAB9078563412);
}
