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
 * File:   Memory_Test.cc
 * Author: wigginsj
 *
 * Created on September 26, 2016, 9:28 AM
 */

#include "Memory_Test.hh"

using namespace karabo::data;
using namespace karabo::xms;


CPPUNIT_TEST_SUITE_REGISTRATION(Memory_Test);


Memory_Test::Memory_Test() {}


Memory_Test::~Memory_Test() {}


void Memory_Test::setUp() {
    m_channelId = Memory::registerChannel();
    m_chunkId = Memory::registerChunk(m_channelId);
}


void Memory_Test::tearDown() {
    Memory::unregisterChannel(m_channelId);
}


void Memory_Test::testSimpleReadAndWrite() {
    const Hash data("a", 42, "b", 3.14, "c", "Karabo");
    Hash readData;

    {
        Memory::write(data, m_channelId, m_chunkId, Memory::MetaData("fooSource", karabo::data::Timestamp()));
        CPPUNIT_ASSERT(Memory::size(m_channelId, m_chunkId) != 0);

        Memory::read(readData, 0, m_channelId, m_chunkId);
        CPPUNIT_ASSERT(readData == data);

        Memory::clearChunkData(m_channelId, m_chunkId);
        CPPUNIT_ASSERT(Memory::size(m_channelId, m_chunkId) == 0);
    }
}

void Memory_Test::testModifyAfterWrite() {
    Hash writeData("a", 1111);
    Hash readData;

    {
        Memory::write(writeData, m_channelId, m_chunkId, Memory::MetaData("fooSource", karabo::data::Timestamp()));
        writeData.set<int>("a", 9999);
        Memory::read(readData, 0, m_channelId, m_chunkId);

        CPPUNIT_ASSERT(readData.get<int>("a") != writeData.get<int>("a"));
        CPPUNIT_ASSERT(readData.get<int>("a") == 1111);

        Memory::clearChunkData(m_channelId, m_chunkId);
    }
}
