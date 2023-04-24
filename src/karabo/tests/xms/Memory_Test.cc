/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Memory_Test.cc
 * Author: wigginsj
 *
 * Created on September 26, 2016, 9:28 AM
 */

#include "Memory_Test.hh"

using namespace karabo::util;
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
        Memory::write(data, m_channelId, m_chunkId, Memory::MetaData("fooSource", karabo::util::Timestamp()));
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
        Memory::write(writeData, m_channelId, m_chunkId, Memory::MetaData("fooSource", karabo::util::Timestamp()));
        writeData.set<int>("a", 9999);
        Memory::read(readData, 0, m_channelId, m_chunkId);

        CPPUNIT_ASSERT(readData.get<int>("a") != writeData.get<int>("a"));
        CPPUNIT_ASSERT(readData.get<int>("a") == 1111);

        Memory::clearChunkData(m_channelId, m_chunkId);
    }
}