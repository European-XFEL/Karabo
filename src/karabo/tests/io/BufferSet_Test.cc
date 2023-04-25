/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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
    bufferCopy.emplaceBack(boost::make_shared<BufferSet::BufferType>(bufferSize, 1));
    // a raw data buffer without writing its size first
    bufferCopy.emplaceBack(
          karabo::util::ByteArray(boost::shared_ptr<char>(new char[bufferSize], boost::checked_array_deleter<char>()),
                                  bufferSize),
          false);
    // a raw data buffer with writing its size first (as unsigned int)
    bufferCopy.emplaceBack(
          karabo::util::ByteArray(boost::shared_ptr<char>(new char[bufferSize], boost::checked_array_deleter<char>()),
                                  bufferSize),
          true);
    // another simple vector buffer at the end
    bufferCopy.emplaceBack(boost::make_shared<BufferSet::BufferType>(bufferSize, 2));

    CPPUNIT_ASSERT_EQUAL(4 * bufferSize + sizeof(unsigned int), bufferCopy.totalSize());
    bufferCopy.appendTo(boostBuffers);
    CPPUNIT_ASSERT_EQUAL(expectedNumBuff, boostBuffers.size());
    boostBuffers.clear();

    BufferSet bufferNoCopy(false);
    // a simple vector buffer
    bufferNoCopy.emplaceBack(boost::make_shared<BufferSet::BufferType>(bufferSize, 1));
    // a raw data buffer without writing its size first
    bufferNoCopy.emplaceBack(
          karabo::util::ByteArray(boost::shared_ptr<char>(new char[bufferSize], boost::checked_array_deleter<char>()),
                                  bufferSize),
          false);
    // a raw data buffer with writing its size first (as unsigned int)
    bufferNoCopy.emplaceBack(
          karabo::util::ByteArray(boost::shared_ptr<char>(new char[bufferSize], boost::checked_array_deleter<char>()),
                                  bufferSize),
          true);
    // another simple vector buffer at the end
    bufferNoCopy.emplaceBack(boost::make_shared<BufferSet::BufferType>(bufferSize, 2));

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
