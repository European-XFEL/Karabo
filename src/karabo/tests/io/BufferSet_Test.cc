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
 * File:   BufferSet_Test.cc
 * Author: flucke
 *
 * Created on April 12, 2019, 1:34 PM
 */

#include "BufferSet_Test.hh"

#include <boost/asio/buffer.hpp>
#include <boost/smart_ptr/make_shared_array.hpp>

#include "karabo/io/BufferSet.hh"

using karabo::io::BufferSet;

CPPUNIT_TEST_SUITE_REGISTRATION(BufferSet_Test);


BufferSet_Test::BufferSet_Test() {}


BufferSet_Test::~BufferSet_Test() {}


void BufferSet_Test::setUp() {}


void BufferSet_Test::tearDown() {}


void BufferSet_Test::testEmplaceAppend() {
    const size_t bufferSize = 100u;
    // While we always emplace 4 buffers, there is one extra to hold the size of the
    // second ByteArray where we use emplaceBack(..., true) after adding a ByteArray before:
    const size_t expectedNumBuff = 5u;
    std::vector<boost::asio::mutable_buffer> boostBuffers;

    BufferSet bufferCopy(true);

    // a simple vector buffer
    bufferCopy.emplaceBack(std::make_shared<BufferSet::BufferType>(bufferSize, 1));
    // a raw data buffer without writing its size first
    bufferCopy.emplaceBack(
          karabo::data::ByteArray(std::shared_ptr<char>(new char[bufferSize], std::default_delete<char[]>()),
                                  bufferSize),
          false);
    // a raw data buffer with writing its size first (as unsigned int)
    bufferCopy.emplaceBack(
          karabo::data::ByteArray(std::shared_ptr<char>(new char[bufferSize], std::default_delete<char[]>()),
                                  bufferSize),
          true);
    // another simple vector buffer at the end
    bufferCopy.emplaceBack(std::make_shared<BufferSet::BufferType>(bufferSize, 2));

    CPPUNIT_ASSERT_EQUAL(4 * bufferSize + sizeof(unsigned int), bufferCopy.totalSize());
    bufferCopy.appendTo(boostBuffers);
    CPPUNIT_ASSERT_EQUAL(expectedNumBuff, boostBuffers.size());
    boostBuffers.clear();

    BufferSet bufferNoCopy(false);
    // a simple vector buffer
    bufferNoCopy.emplaceBack(std::make_shared<BufferSet::BufferType>(bufferSize, 1));
    // a raw data buffer without writing its size first
    bufferNoCopy.emplaceBack(
          karabo::data::ByteArray(std::shared_ptr<char>(new char[bufferSize], std::default_delete<char[]>()),
                                  bufferSize),
          false);
    // a raw data buffer with writing its size first (as unsigned int)
    bufferNoCopy.emplaceBack(
          karabo::data::ByteArray(std::shared_ptr<char>(new char[bufferSize], std::default_delete<char[]>()),
                                  bufferSize),
          true);
    // another simple vector buffer at the end
    bufferNoCopy.emplaceBack(std::make_shared<BufferSet::BufferType>(bufferSize, 2));

    CPPUNIT_ASSERT_EQUAL(4 * bufferSize + sizeof(unsigned int), bufferNoCopy.totalSize());
    bufferNoCopy.appendTo(boostBuffers);
    CPPUNIT_ASSERT_EQUAL(expectedNumBuff, boostBuffers.size());
    boostBuffers.clear();


    bufferCopy.appendTo(bufferNoCopy, false); // no copies

    CPPUNIT_ASSERT_EQUAL(8 * bufferSize + 2 * sizeof(unsigned int), bufferNoCopy.totalSize());
    bufferNoCopy.appendTo(boostBuffers);
    CPPUNIT_ASSERT_EQUAL(2 * expectedNumBuff, boostBuffers.size());
    boostBuffers.clear();

    bufferNoCopy.appendTo(bufferCopy, false); // no copies

    CPPUNIT_ASSERT_EQUAL(12 * bufferSize + 3 * sizeof(unsigned int), bufferCopy.totalSize());
    bufferCopy.appendTo(boostBuffers);
    CPPUNIT_ASSERT_EQUAL(3 * expectedNumBuff, boostBuffers.size());
    boostBuffers.clear();
}
