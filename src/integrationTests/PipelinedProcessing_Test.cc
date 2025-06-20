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
 * File:   PipelineProcessing_Test.cc
 * Author: haufs
 *
 * Modified by J. Zhu
 *
 * Created on Sep 20, 2016, 3:40:34 PM
 */

// Added to trigger integration tests

#include "PipelinedProcessing_Test.hh"

#include <boost/format.hpp>
#include <chrono>
#include <karabo/log/Logger.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/xms/Memory.hh>

#include "CppUnitMacroExtension.hh"

using namespace std::chrono;
using namespace std::literals::chrono_literals;
using namespace std::string_literals; // For '"abc"s'
USING_KARABO_NAMESPACES;


CPPUNIT_TEST_SUITE_REGISTRATION(PipelinedProcessing_Test);


PipelinedProcessing_Test::PipelinedProcessing_Test() = default;


PipelinedProcessing_Test::~PipelinedProcessing_Test() = default;


void PipelinedProcessing_Test::setUp() {
    // set broker
    // putenv("KARABO_BROKER=tcp://localhost:7777");

    // Start central event-loop
    auto work = [](std::stop_token stoken) {
        try {
            EventLoop::work();
        } catch (const karabo::data::TimeoutException& e) {
            // Looks like thread joining fails sometimes...
            std::clog << "Timeout from EventLoop::work(): " << e << std::endl;
        }
    };
    m_eventLoopThread = std::jthread(work);

    // Create and start server
    Hash config("serverId", m_server, "log.level", "ERROR");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();

    // Create client
    m_deviceClient = std::shared_ptr<DeviceClient>(new DeviceClient());
}


void PipelinedProcessing_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();
    EventLoop::stop();
}


void PipelinedProcessing_Test::appTestRunner() {
    testInputConnectionTracking();
    // in order to avoid recurring setup and tear down calls, all tests are run in a single runner
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender));
    m_nDataPerRun = m_deviceClient->get<unsigned int>(m_sender, "nData");

    testGetOutputChannelSchema();

    testPipeWait();

    testPipeWaitPerf();

    testPipeDrop();

    testPipeQueue();

    testPipeQueueAtLimit();

    testPipeMinData();

    testPipeTwoPots();

    // After this test, the sender will have "output1.noInputShared" == "wait".
    testPipeTwoSharedReceiversWait();

    // Test assumes "output1.noInputShared" == "wait".
    testSharedReceiversSelector();

    // After test it will be back "output1.noInputShared == drop".
    testPipeTwoSharedReceiversDrop();

    // test restarts m_sender
    testPipeTwoSharedReceiversQueue();

    // test restarts m_sender
    testPipeTwoSharedReceiversQueueAtLimit();

    testQueueClearOnDisconnect();

    // this test uses output2 channel of the sender
    testProfileTransferTimes();

    killDeviceWithAssert(m_sender);
}


void PipelinedProcessing_Test::testInputConnectionTracking() {
    std::clog << "---\ntestInputConnectionTracking\n" << std::flush;

    Hash config(m_receiverBaseConfig);
    config.set("deviceId", m_receiver);
    instantiateDeviceWithAssert("PipeReceiverDevice", config);

    const auto& desiredConnections = config.get<std::vector<std::string>>("input.connectedOutputChannels");
    // Empty default would render this test useless, so ensure that it is not empty:
    CPPUNIT_ASSERT(!desiredConnections.empty());

    // In the beginning, there is no connection (sender not yet up), so all is missing:
    CPPUNIT_ASSERT_EQUAL(desiredConnections,
                         m_deviceClient->get<std::vector<std::string>>(m_receiver, "input.missingConnections"));

    // After instantiation of sender, receiver connects and "documents" that no connection is missing anymore:
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender));
    CPPUNIT_ASSERT(pollDeviceProperty(m_receiver, "input.missingConnections", std::vector<std::string>()));

    // After killing the sender again, receiver's input channel misses it again:
    killDeviceWithAssert(m_sender);
    CPPUNIT_ASSERT(pollDeviceProperty(m_receiver, "input.missingConnections", desiredConnections));

    // Leave a clean state
    killDeviceWithAssert(m_receiver);

    std::clog << "Passed!" << std::endl;
}


void PipelinedProcessing_Test::testGetOutputChannelSchema() {
    std::clog << "---\ntestGetOutputChannelSchema\n";

    karabo::data::Hash dataSchema = m_deviceClient->getOutputChannelSchema(m_sender, "output1");

    CPPUNIT_ASSERT(dataSchema.has("dataId"));
    CPPUNIT_ASSERT(dataSchema.getType("dataId") == karabo::data::Types::INT32);
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("dataId", KARABO_SCHEMA_VALUE_TYPE) == "INT32");
    CPPUNIT_ASSERT(dataSchema.has("data"));
    CPPUNIT_ASSERT(dataSchema.getType("data") == karabo::data::Types::INT32);
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("data", KARABO_SCHEMA_VALUE_TYPE) == "VECTOR_INT64");
    CPPUNIT_ASSERT(dataSchema.has("array"));
    CPPUNIT_ASSERT(!dataSchema.hasAttribute(
          "array", KARABO_HASH_CLASS_ID)); // As a Schema it should not carry info about HASH_CLASS_ID
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array", KARABO_SCHEMA_CLASS_ID) == "NDArray");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.data", KARABO_SCHEMA_VALUE_TYPE) == "BYTE_ARRAY");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.shape", KARABO_SCHEMA_VALUE_TYPE) == "VECTOR_UINT64");
    CPPUNIT_ASSERT(dataSchema.getAttributeAs<std::string>("array.shape", KARABO_SCHEMA_DEFAULT_VALUE) == "100,200,0");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.type", KARABO_SCHEMA_VALUE_TYPE) == "INT32");
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(Types::UINT8),
                         dataSchema.getAttribute<int>("array.type", KARABO_SCHEMA_DEFAULT_VALUE));
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.isBigEndian", KARABO_SCHEMA_VALUE_TYPE) == "BOOL");
    CPPUNIT_ASSERT(dataSchema.getAttributeAs<std::string>("array.isBigEndian", KARABO_SCHEMA_DEFAULT_VALUE) == "0");

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeWait() {
    std::clog << "---\ntestPipeWait (onSlowness = 'wait')\n";

    const auto testStartTime = high_resolution_clock::now();

    // use only one receiver for a group of tests
    karabo::data::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "input.onSlowness", "wait");
    instantiateDeviceWithAssert("PipeReceiverDevice", config);
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));

    // printSenderOutputChannelConnections("testPipeWait");
    testSenderOutputChannelConnections(1UL, {m_receiver + ":input"}, "copy", "wait", "local", {m_receiver + ":input2"},
                                       "copy", "drop", "local");

    testPipeWait(0, 0);
    testPipeWait(100, 0);
    testPipeWait(0, 100);

    killDeviceWithAssert(m_receiver);

    std::clog << "Test duration (ms): "
              << duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count() << std::endl;

    testSenderOutputChannelConnections();

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testSenderOutputChannelConnections(
      size_t tsize, const std::vector<std::string>& receivers1, const std::string& distrib1,
      const std::string& slowness1, const std::string& mloc1, const std::vector<std::string>& receivers2,
      const std::string& distrib2, const std::string& slowness2, const std::string& mloc2) {
    std::vector<karabo::data::Hash> output1, output2;

    // It is impossible to guarantee that the connection is already established and the device properties are updated in
    // the m_deviceClient when this function is called. In a busy system it may be that the first connection attempt
    // fails on TCP level (though no proof that this ever happened...). To be on the safe side, we wait long enough that
    // the reconnection cycle in SignalSlotable can fix the issue with a reconnection attempt.
    int waitMs = 10000; // 6 seconds is the pipeline reconnection cycle in SignalSlotable
    while (waitMs > 0) {
        m_deviceClient->get(m_sender, "output1.connections", output1);
        m_deviceClient->get(m_sender, "output2.connections", output2);
        if (tsize == output1.size() && tsize == output2.size()) {
            break;
        }
        waitMs -= 50;
        std::this_thread::sleep_for(50ms);
    }
    CPPUNIT_ASSERT_EQUAL(tsize, output1.size());
    CPPUNIT_ASSERT_EQUAL(tsize, output2.size());

    for (size_t i = 0; i < output1.size(); ++i) {
        const std::string& remoteId = output1[i].get<std::string>("remoteId");
        CPPUNIT_ASSERT(std::find(receivers1.begin(), receivers1.end(), remoteId) != receivers1.end());
        CPPUNIT_ASSERT_EQUAL(distrib1, output1[i].get<std::string>("dataDistribution"));
        CPPUNIT_ASSERT_EQUAL(slowness1, output1[i].get<std::string>("onSlowness"));
        CPPUNIT_ASSERT_EQUAL(mloc1, output1[i].get<std::string>("memoryLocation"));
    }

    for (size_t i = 0; i < output2.size(); ++i) {
        const std::string& remoteId = output2[i].get<std::string>("remoteId");
        CPPUNIT_ASSERT(std::find(receivers2.begin(), receivers2.end(), remoteId) != receivers2.end());
        CPPUNIT_ASSERT_EQUAL(distrib2, output2[i].get<std::string>("dataDistribution"));
        CPPUNIT_ASSERT_EQUAL(slowness2, output2[i].get<std::string>("onSlowness"));
        CPPUNIT_ASSERT_EQUAL(mloc2, output2[i].get<std::string>("memoryLocation"));
    }
}


void PipelinedProcessing_Test::printSenderOutputChannelConnections(const std::string& name) {
    std::vector<karabo::data::Hash> output1;
    m_deviceClient->get(m_sender, "output1.connections", output1);
    std::clog << name << " : printSenderOutputChannelConnections output1.connections.size = " << output1.size()
              << std::endl;
    for (std::vector<karabo::data::Hash>::const_iterator it1 = output1.begin(); it1 != output1.end(); ++it1) {
        std::clog << name << " : printSenderOutputChannelConnections output1.connection:\t" << *it1 << std::endl;
    }

    std::vector<karabo::data::Hash> output2;
    m_deviceClient->get(m_sender, "output2.connections", output2);
    std::clog << name << " : printSenderOutputChannelConnections output2.connections.size = " << output2.size()
              << std::endl;
    for (std::vector<karabo::data::Hash>::const_iterator it2 = output2.begin(); it2 != output2.end(); ++it2) {
        std::clog << name << " : printSenderOutputChannelConnections output2.connection:\t" << *it2 << std::endl;
    }
}


void PipelinedProcessing_Test::testPipeWait(unsigned int processingTime, unsigned int delayTime) {
    std::clog << "- processingTime = " << processingTime << " ms, delayTime = " << delayTime << " ms\n";

    m_deviceClient->set(m_receiver, "processingTime", processingTime);
    m_deviceClient->set(m_sender, "delay", delayTime);

    int64_t elapsedTimeIn_microseconds = 0ll;
    // we use a single receiver device for several successive tests.
    unsigned int nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    unsigned int nTotalDataOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
    unsigned int nDataExpected = nTotalData0;
    for (unsigned int nRun = 0; nRun < m_numRunsPerTest; ++nRun) {
        const auto startTimepoint = high_resolution_clock::now();

        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));
        // Then call its slot
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        nDataExpected += m_nDataPerRun;
        // And poll for the correct answer
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataExpected));

        const auto dur = high_resolution_clock::now() - startTimepoint;
        // Note that duration contains overhead from message travel time and polling interval in pollDeviceProperty!
        elapsedTimeIn_microseconds += duration_cast<microseconds>(dur).count();

        // Check that EOS handling is not called too early

        // EOS comes a bit later, so we have to poll client again to be sure...
        // Note: only one EOS arrives after receiving a train of data!
        pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected);
        CPPUNIT_ASSERT_EQUAL(nDataExpected, m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos"));

        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string>>(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1[0], sources[0]);
        // Check that receiver did not post any problem on status:
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver, "status"));
        // This only can be tested if we used an input handler and not onData
        if (!m_deviceClient->get<bool>(m_receiver, "onData")) {
            auto sources = m_deviceClient->get<std::vector<std::string>>(m_receiver, "dataSourcesFromIndex");
            CPPUNIT_ASSERT_EQUAL(m_senderOutput1[0], sources[0]);
        }
    }

    const unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    // Note that this measurement checks the inner-process shortcut - and includes timing overhead e.g.
    // pollDeviceProperty In addition, the process and delay times also affect mbps.
    std::clog << "  summary: Megabytes per sec : " << mbps
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds << ", dataItemSize = " << dataItemSize
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl;
}


void PipelinedProcessing_Test::testPipeWaitPerf() {
    std::clog << "---\ntestPipeWaitPerf (onSlowness = 'wait', senderDelay = 0, receiverProcessing = 0)\n";
    karabo::data::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "input.onSlowness", "wait");
    instantiateDeviceWithAssert("PipeReceiverDevice", config);
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));

    testSenderOutputChannelConnections(1UL, {m_receiver + ":input"}, "copy", "wait", "local", {m_receiver + ":input2"},
                                       "copy", "drop", "local");

    // We are looking for measures of pipeline data "transmission" performance - zero all
    // sender's delays and receivers processing times so we can focus on transmission times.
    unsigned int currSendDelay = m_deviceClient->get<unsigned int>(m_sender, "delay");
    m_deviceClient->set(m_receiver, "processingTime", 0u);
    m_deviceClient->set(m_sender, "delay", 0u);

    testPipeWaitPerf(5000);

    // Restores the sender's delay before leaving.
    m_deviceClient->set(m_sender, "delay", currSendDelay);

    killDeviceWithAssert(m_receiver);

    testSenderOutputChannelConnections();

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeWaitPerf(unsigned int numOfDataItems) {
    std::clog << "- numOfDataItems = " << numOfDataItems << std::endl;

    // Stores the current num of data items per run; all the other tests in the suite
    // use the same value, which is the value of the sender's nData property.
    unsigned int currItems = m_deviceClient->get<unsigned int>(m_sender, "nData");
    m_deviceClient->set<unsigned int>(m_sender, "nData", numOfDataItems);

    int64_t elapsedTimeIn_microseconds = 0ll;
    // we use a single receiver device for several successive tests.
    unsigned int nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    unsigned int nTotalDataOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
    unsigned int nDataExpected = nTotalData0;

    // make sure the sender has stopped sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));
    // Then call its slot
    const auto startTimepoint = high_resolution_clock::now();
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    nDataExpected += numOfDataItems;
    // And poll for the correct answer
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataExpected, true, 40000));

    const auto dur = high_resolution_clock::now() - startTimepoint;
    // Note that duration contains overhead from message travel time and polling interval in pollDeviceProperty!
    elapsedTimeIn_microseconds += duration_cast<microseconds>(dur).count();

    // EOS comes a bit later, so we have to poll client again to be sure...
    // Note: only one EOS arrives after receiving a train of data!
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected));

    // Check that receiver did not post any problem on status:
    CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver, "status"));

    const unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    // The process and delay times also affect mbps.
    std::clog << "  summary: Megabytes per sec = " << mbps << std::endl
              << "           total time (microsends) = " << elapsedTimeIn_microseconds << std::endl
              << "           data item size (bytes) = " << dataItemSize << std::endl
              << "           nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl
              << std::endl;

    // Restores the number of data items per run before leaving.
    m_deviceClient->set<unsigned int>(m_sender, "nData", currItems);
}


void PipelinedProcessing_Test::testPipeDrop() {
    std::clog << "---\ntestPipeDrop (onSlowness = 'drop')\n";

    const auto testStartTime = high_resolution_clock::now();

    karabo::data::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "input.onSlowness", "drop");
    instantiateDeviceWithAssert("PipeReceiverDevice", config);
    CPPUNIT_ASSERT_EQUAL(std::string("drop"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));

    testSenderOutputChannelConnections(1UL, {m_receiver + ":input"}, "copy", "drop", "local", {m_receiver + ":input2"},
                                       "copy", "drop", "local");

    testPipeDrop(10, 0, true);
    testPipeDrop(100, 0, true);
    testPipeDrop(0, 100, false);

    killDeviceWithAssert(m_receiver);

    std::clog << "Test duration (ms): "
              << duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count() << std::endl;

    testSenderOutputChannelConnections();

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeDrop(unsigned int processingTime, unsigned int delayTime, bool dataLoss) {
    std::clog << "- processingTime = " << processingTime << " ms, delayTime = " << delayTime << " ms\n";

    m_deviceClient->set(m_receiver, "processingTime", processingTime);
    m_deviceClient->set(m_sender, "delay", delayTime);

    size_t elapsedTimeIn_microseconds = 0;
    unsigned int nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    unsigned int nTotalDataOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
    unsigned int nDataExpected = nTotalData0;
    for (unsigned int nRun = 0; nRun < m_numRunsPerTest; ++nRun) {
        auto startTimepoint = high_resolution_clock::now();
        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        // test data
        if (!dataLoss) {
            nDataExpected += m_nDataPerRun;
            CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataExpected));
        } else {
            // poll until nTotalDataOnEos changes (increases)
            CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected, false));

            // if the processing time is comparable to or larger than the delay time,
            // the number of received data is random, but should be larger than the
            // number of local buffers (currently one 'active' and one 'inactive')
            unsigned int nTotalData = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
            CPPUNIT_ASSERT(nTotalData < nDataExpected + m_nDataPerRun);
            CPPUNIT_ASSERT(nTotalData >= nDataExpected + m_nPots);
            nDataExpected = nTotalData;
        }

        auto dur = high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += duration_cast<microseconds>(dur).count();

        // test EOS
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected));

        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string>>(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1[0], sources[0]);
    }

    unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    std::clog << "  summary: Megabytes per sec : " << mbps
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds << ", dataItemSize = " << dataItemSize
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl;
}


void PipelinedProcessing_Test::testPipeQueue() {
    std::clog << "---\ntestPipeQueue (onSlowness = 'queueDrop', dataDistribution = 'copy')\n";

    const auto testStartTime = high_resolution_clock::now();

    karabo::data::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "input", Hash("onSlowness", "queueDrop", "maxQueueLength", 1000u));
    config += Hash("deviceId", m_receiver, "input.dataDistribution", "copy");
    instantiateDeviceWithAssert("PipeReceiverDevice", config);
    CPPUNIT_ASSERT_EQUAL(std::string("queueDrop"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("copy"), m_deviceClient->get<std::string>(m_receiver, "input.dataDistribution"));

    testSenderOutputChannelConnections(1UL, {m_receiver + ":input"}, "copy", "queueDrop", "local",
                                       {m_receiver + ":input2"}, "copy", "drop", "local");

    // Higher processing times are used to allow observation that data is sent faster than it is
    // handled by the receiver.
    testPipeQueue(50, 5);
    // Higher delay times are used to allow observation that the sender becomes the bottleneck in
    // those scenarios.
    testPipeQueue(5, 50);

    killDeviceWithAssert(m_receiver);

    std::clog << "Test duration (ms): "
              << duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count() << std::endl;

    testSenderOutputChannelConnections();

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeQueue(unsigned int processingTime, unsigned int delayTime) {
    std::clog << "- processingTime = " << processingTime << " ms, delayTime = " << delayTime << " ms\n";

    m_deviceClient->set(m_receiver, "processingTime", processingTime);
    m_deviceClient->set(m_sender, "delay", delayTime);

    int64_t elapsedTimeIn_microseconds = 0ll;
    // we use a single receiver device for several successive tests.
    unsigned int nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    unsigned int nTotalDataOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
    unsigned int nDataExpected = nTotalData0;
    for (unsigned int nRun = 0; nRun < m_numRunsPerTest; ++nRun) {
        const auto startTimepoint = high_resolution_clock::now();

        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));

        // Then call its slot again
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        // Makes sure the sender has finished sending the data in this run. We can't rely on 'currentDataId' for
        // this because in situations of high delayTime a pollDeviceProperty polling can return immediately due to
        // an expected value from the previous run.
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));

        nDataExpected += m_nDataPerRun;
        if (processingTime > 2 * delayTime) {
            // If processingTime is significantly bigger than delayTime, we are bound by processingTime. In this scena-
            // rio, the sender will start "sending" (actually dispatching data to a queue) immediately. As the receiver
            // has a relatively large processingTime, it will take a while for the receiver to actually receive the
            // data. We assert a maximum ratio of data arrival in this scenario.
            const unsigned int receivedSoFar = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
            CPPUNIT_ASSERT_MESSAGE(
                  karabo::data::toString(receivedSoFar) + " " + karabo::data::toString(nDataExpected),
                  2 * (nDataExpected - receivedSoFar) <= m_nDataPerRun * 3); // at max. 2/3 have arrived
        } else if (2 * processingTime < delayTime) {
            // If delayTime is significantly bigger than the processing time, we are bound by the delayTime. This means
            // that between two successive data writes the sender will wait delayTime milliseconds and the sender is
            // expected to be slowest part.
            // We assert that the amount of received data must be at the maximum m_nPots lower than the amount of
            // expected sent data (no bottleneck on the receiver).
            const unsigned int receivedSoFar = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
            CPPUNIT_ASSERT_MESSAGE(karabo::data::toString(receivedSoFar) + " " + karabo::data::toString(nDataExpected),
                                   nDataExpected - receivedSoFar <= m_nPots);
        }
        // In the end, all should arrive
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataExpected));

        const auto dur = high_resolution_clock::now() - startTimepoint;
        // Note that duration contains overhead from message travel time and polling interval in pollDeviceProperty!
        elapsedTimeIn_microseconds += duration_cast<microseconds>(dur).count();

        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected));

        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string>>(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1[0], sources[0]);
        // Check that receiver did not post any problem on status:
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver, "status"));
        // This only can be tested if we used an input handler and not onData
        if (!m_deviceClient->get<bool>(m_receiver, "onData")) {
            auto sources = m_deviceClient->get<std::vector<std::string>>(m_receiver, "dataSourcesFromIndex");
            CPPUNIT_ASSERT_EQUAL(m_senderOutput1[0], sources[0]);
        }
    }

    m_deviceClient->set(m_sender, "delay", 0u); // Restore the sender's delay parameter back to its default.

    const unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    // Note that this measurement checks the inner-process shortcut - and includes timing overhead e.g.
    // pollDeviceProperty In addition, the process and delay times also affect mbps.
    std::clog << "  summary: Megabytes per sec : " << mbps
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds << ", dataItemSize = " << dataItemSize
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl;
}


void PipelinedProcessing_Test::testPipeQueueAtLimit() {
    // 1) Test specifically configured queue length - here the default
    const unsigned int maxLengthCfg = 100u;

    // Receiver processing time much higher than sender delay between data sending:
    // Data will be queued until queue is full and then drop some data
    testPipeQueueAtLimit(2, 0, maxLengthCfg, true, true); // true, true ==> expectDataLoss, slowReceiver

    // If sender delay time much higher than the receiver processing time, no data loss despite the queueDrop option
    // (the 'slowReceiver == false' test fails sometimes with delay = 2, so 4 was choosen - but even that failed in
    //  https://git.xfel.eu/Karabo/Framework/-/jobs/238881)
    testPipeQueueAtLimit(0, 7, maxLengthCfg, false, false);

    // 2) Test overall limit from Memory, no effective queue length limit (queue can be as big as the whole Memory
    // buffer of the Output Channel).

    // Receiver processing time much higher than sender delay between data sending:
    // Data will be queued until queue is full and then drop some data
    testPipeQueueAtLimit(5, 0, karabo::xms::Memory::MAX_N_CHUNKS, true,
                         true); // true, true ==> expectDataLoss, slowReceiver

    // If sender delay time much higher than the receiver processing time, no data loss despite the queueDrop option
    // (the 'slowReceiver == false' test fails sometimes with delay = 2, so choose 4)
    testPipeQueueAtLimit(0, 4, karabo::xms::Memory::MAX_N_CHUNKS, false, false);
}


void PipelinedProcessing_Test::testPipeQueueAtLimit(unsigned int processingTime, unsigned int delayTime,
                                                    unsigned int activeQueueLimit, bool expectDataLoss,
                                                    bool slowReceiver) {
    std::clog << "---\ntestPipeQueueAtLimit (onSlowness = 'queueDrop', dataDistribution = 'copy') "
              << "- processingTime = " << processingTime << " ms, delayTime = " << delayTime << " ms, "
              << "activeQueueLimit = " << activeQueueLimit << "\n";

    karabo::data::Hash config(m_receiverBaseConfig);
    config.merge(Hash("deviceId", m_receiver, "input.onSlowness", "queueDrop", "input.dataDistribution", "copy",
                      "input.maxQueueLength", activeQueueLimit));
    instantiateDeviceWithAssert("PipeReceiverDevice", config);
    CPPUNIT_ASSERT_EQUAL("queueDrop"s, m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL("copy"s, m_deviceClient->get<std::string>(m_receiver, "input.dataDistribution"));
    CPPUNIT_ASSERT_EQUAL(activeQueueLimit, m_deviceClient->get<unsigned int>(m_receiver, "input.maxQueueLength"));

    testSenderOutputChannelConnections(1UL, {m_receiver + ":input"}, "copy", "queueDrop", "local",
                                       {m_receiver + ":input2"}, "copy", "drop", "local");


    m_deviceClient->set(m_receiver, "processingTime", processingTime);
    const int prev_delay = m_deviceClient->get<unsigned int>(m_sender, "delay");
    const int prev_nData = m_deviceClient->get<unsigned int>(m_sender, "nData");
    const int prev_dataSize = m_deviceClient->get<unsigned int>(m_sender, "dataSize");
    // We need a lot of data to fill up the queue so that data is indeed dropped
    const unsigned int nData = activeQueueLimit + 1000u;
    const unsigned int dataSize = 1000u; // else memory trouble with big queues on small memory machines
    m_deviceClient->set(m_sender, Hash("delay", delayTime, "nData", nData, "dataSize", dataSize));

    const unsigned int nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    const unsigned int nTotalDataOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
    const unsigned int nDataExpected = nTotalData0 + nData; // expected if nothing dropped

    const auto testStartTime = high_resolution_clock::now();
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    // Makes sure the sender has finished sending the data in this run
    // (do not wait for state NORMAL, see testPipeTwoSharedReceiversQueueDrop).
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_sender, "currentDataId", nData - 1u, true,
                                                    m_maxTestTimeOut * 4)); // Longer time out due to many data items

    unsigned int receivedWhenWriteDone = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    if (slowReceiver) {
        // At least some data has already been processed
        // Though there is no guarantee - seen a CI with none of 1100 received, so wait a bit if needed
        int nTries = 100;
        while (0 == receivedWhenWriteDone && --nTries >= 0) {
            std::this_thread::sleep_for(10ms);
            receivedWhenWriteDone = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
        }
        CPPUNIT_ASSERT(receivedWhenWriteDone > 0);
    } else {
        // No bottleneck on the receiver, i.e. all is received, except what maybe sits in the pots of the buffer
        const unsigned int missing = nDataExpected - receivedWhenWriteDone;
        CPPUNIT_ASSERT_LESSEQUAL(m_nPots, missing);
    }
    // When EOS have arrived (and thus all data), "nTotalDataOnEos" is set to a new value.
    // So we wait here until that happens - and then stop timer
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nTotalDataOnEos0, false));
    auto durMs = duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count();

    const unsigned int nTotalDataEnd = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    const unsigned int nTotalDataOnEos = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
    // These are the same - but maybe not nDataExpected
    CPPUNIT_ASSERT_EQUAL(nTotalDataEnd, nTotalDataOnEos);

    if (expectDataLoss) {
        // If the receiver is very slow, data is dropped Note: dropped only if queue was full, > 2000 items!
        CPPUNIT_ASSERT_LESS(nDataExpected, nTotalDataEnd);
        // But at least the queue length arrived
        CPPUNIT_ASSERT_GREATER(activeQueueLimit, nTotalDataEnd);
    } else {
        // Sender is bottleneck? Or queue and wait if queue full? All data arrived!
        CPPUNIT_ASSERT_EQUAL(nDataExpected, nTotalDataEnd);
    }

    // Check that receiver did not post any problem on status:
    CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver, "status"));

    killDeviceWithAssert(m_receiver);
    testSenderOutputChannelConnections();
    // Restore the sender's parameters back to their defaults.
    m_deviceClient->set(m_sender, Hash("delay", prev_delay, "nData", prev_nData, "dataSize", prev_dataSize));
    std::clog << "   Success - test duration " << durMs << " ms: " << "n(data_sent) = " << nData
              << ", n(data_arrived_when_all_sent) " << receivedWhenWriteDone - nTotalData0
              << ", n(data_arrived_end) = " << nTotalDataEnd - nTotalData0 << std::endl;
}


void PipelinedProcessing_Test::testPipeMinData() {
    std::clog << "---\ntestPipeMinData\n";

    const unsigned int originalSenderDelay = m_deviceClient->get<unsigned int>(m_sender, "delay");

    const auto testStartTime = high_resolution_clock::now();

    m_deviceClient->set(m_sender, "delay", 0u);

    // input.minData = 1 by default -- for this test must be divisor of m_nDataPerRun
    const unsigned int minData = 4;
    CPPUNIT_ASSERT_EQUAL(0u, m_nDataPerRun % minData); // see below

    // start a receiver with "input.onData = false", i.e. call PipeReceiverDevice::onInput while reading data,
    // and "minData > 1"
    karabo::data::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "input.onSlowness", "wait", "input.minData", minData);
    instantiateDeviceWithAssert("PipeReceiverDevice", config);

    testSenderOutputChannelConnections(1UL, {m_receiver + ":input"}, "copy", "wait", "local", {m_receiver + ":input2"},
                                       "copy", "drop", "local");

    // make sure the sender has stopped sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));

    // write data asynchronously
    m_deviceClient->execute(m_sender, "write");
    // make sure the sender has started sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::ACTIVE));

    // poll until nTotalDataOnEos changes
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", 0, false, m_maxTestTimeOut));

    // test if data source was correctly passed
    auto sources = m_deviceClient->get<std::vector<std::string>>(m_receiver, "dataSourcesFromIndex");
    // test that "input.onData = false" and "input.minData" are respected
    // here we test only the last call of onInput - if minData is not a divisor of m_nDataPerRun, the check fails
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(minData), sources.size());
    for (auto& src : sources) {
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1[0], src);
    }

    CPPUNIT_ASSERT_EQUAL(m_nDataPerRun, m_deviceClient->get<unsigned int>(m_receiver, "nTotalData"));

    killDeviceWithAssert(m_receiver);

    // Now check minData = 0, i.e. call onInput only when endOfStream is received
    config.set("input.minData", 0u);
    instantiateDeviceWithAssert("PipeReceiverDevice", config);

    testSenderOutputChannelConnections(1UL, {m_receiver + ":input"}, "copy", "wait", "local", {m_receiver + ":input2"},
                                       "copy", "drop", "local");

    // make sure the sender has stopped sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));

    // write data asynchronously
    m_deviceClient->execute(m_sender, "write");
    // make sure the sender has started sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::ACTIVE));

    // poll until nTotalDataOnEos changes
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", 0, false, m_maxTestTimeOut));

    // onInput was called exactly once with all data
    sources = m_deviceClient->get<std::vector<std::string>>(m_receiver, "dataSourcesFromIndex");
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(m_nDataPerRun), sources.size());
    for (auto& src : sources) {
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1[0], src);
    }

    killDeviceWithAssert(m_receiver);

    std::clog << "Test duration (ms): "
              << duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count() << std::endl;

    testSenderOutputChannelConnections();

    std::clog << "Passed!\n\n";

    // Restores the sender's delay to the value it had at the beginning of the test.
    m_deviceClient->set(m_sender, "delay", originalSenderDelay);
}


void PipelinedProcessing_Test::testPipeTwoPots() {
    std::clog << "---\ntestPipeTwoPots\n";

    const auto testStartTime = high_resolution_clock::now();

    const unsigned int originalSenderDelay = m_deviceClient->get<unsigned int>(m_sender, "delay");

    // As this test interrupts the sender in the middle of a send of 'nData' data items, it depends on some sender delay
    // to be able to assert precisely how many data items have been sent after the sender 'Stop' slot has been invoked.
    // The delay set in the next line is high enough to make sure that there will be one extra data item left in the
    // unprocessed Pot of the receiver input channel.
    m_deviceClient->set(m_sender, "delay", 75u);

    // start a receiver whose processingTime is significantly longer than the writing time of the output channel
    karabo::data::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "processingTime", 200, "input.onSlowness", "wait");
    instantiateDeviceWithAssert("PipeReceiverDevice", config);

    testSenderOutputChannelConnections(1UL, {m_receiver + ":input"}, "copy", "wait", "local", {m_receiver + ":input2"},
                                       "copy", "drop", "local");

    for (unsigned int nDataWhenStop = 3; nDataWhenStop < 8; ++nDataWhenStop) {
        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));

        // write data asynchronously
        m_deviceClient->execute(m_sender, "write");

        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataWhenStop));
        // stop sending data after receiving nDataWhenStop data!
        m_deviceClient->execute(m_sender, "stop");
        // The receiver is expected to get more data when EOS arrives: one which was already in the inactive pot when
        // the "stop" slot is called and potentially one more in case reporting nTotalData to us was delayed.
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", 0, false));
        const unsigned int nTotalDataOnEos = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
        CPPUNIT_ASSERT_MESSAGE("whenStop: " + toString(nDataWhenStop) += ", whenEos: " + toString(nTotalDataOnEos),
                               nDataWhenStop + 1 == nTotalDataOnEos || nDataWhenStop + 2 == nTotalDataOnEos);

        // reset nTotalData and nTotalDataOnEos
        m_deviceClient->execute(m_receiver, "reset");
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", 0));
    }

    // Restores the sender 'delay' to the value it had at the beginning of the test.
    m_deviceClient->set(m_sender, "delay", originalSenderDelay);

    killDeviceWithAssert(m_receiver);

    std::clog << "Test duration (ms): "
              << duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count() << std::endl;

    testSenderOutputChannelConnections();

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedReceiversWait() {
    std::clog << "---\ntestPipeTwoSharedReceiversWait (onSlowness = 'wait', dataDistribution = 'shared')\n";

    const auto testStartTime = high_resolution_clock::now();

    killDeviceWithAssert(m_sender);
    instantiateDeviceWithAssert("P2PSenderDevice",
                                Hash("deviceId", m_sender, "output1", Hash("noInputShared", "wait")));

    karabo::data::Hash config1(m_receiverBaseConfig);
    config1 += Hash("deviceId", m_receiver1, "input.dataDistribution", "shared");

    karabo::data::Hash config2(config1);
    config2.set<std::string>("deviceId", m_receiver2);

    instantiateDeviceWithAssert("PipeReceiverDevice", config1);
    instantiateDeviceWithAssert("PipeReceiverDevice", config2);

    // check that the default value of dataDistribution is "copy"
    CPPUNIT_ASSERT_EQUAL(std::string("copy"), m_deviceClient->get<std::string>(m_receiver1, "input2.dataDistribution"));
    // check that the default value of noInputShared is "drop"
    CPPUNIT_ASSERT_EQUAL(std::string("drop"), m_deviceClient->get<std::string>(m_sender, "output2.noInputShared"));

    testSenderOutputChannelConnections(2UL, {m_receiver1 + ":input", m_receiver2 + ":input"}, "shared", "drop", "local",
                                       {m_receiver1 + ":input2", m_receiver2 + ":input2"}, "copy", "drop", "local");

    // no losses despite input1.onSlowness is "drop" - for shared distribution "output1.noInputShared" rules
    testPipeTwoSharedReceivers(0, 0, 0, false, false);
    testPipeTwoSharedReceivers(200, 0, 0, false, false);
    testPipeTwoSharedReceivers(100, 100, 0, false, false);

    // Now test the shared input selector code, using round-robin
    m_deviceClient->set(m_sender, "nextSharedInput", "roundRobinSelector");

    testPipeTwoSharedReceivers(0, 0, 20, false, true);
    testPipeTwoSharedReceivers(200, 0, 0, false, true);
    testPipeTwoSharedReceivers(100, 100, 0, false, true);

    // Reset shared input selector
    m_deviceClient->set(m_sender, "nextSharedInput", std::string());

    killDeviceWithAssert(m_receiver1);
    killDeviceWithAssert(m_receiver2);

    std::clog << "Test duration (ms): "
              << duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count() << std::endl;

    testSenderOutputChannelConnections();

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedReceiversDrop() {
    std::clog << "---\ntestPipeTwoSharedReceiversDrop (onSlowness = 'drop', dataDistribution = 'shared')\n";

    // restart the sender with "output1.noInputShared == drop"
    killDeviceWithAssert(m_sender);
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender, "output1.noInputShared", "drop"));

    karabo::data::Hash config1(m_receiverBaseConfig);
    // set onSlowness to "wait" - to demonstrate that it does not matter
    config1 += Hash("deviceId", m_receiver1, "input.dataDistribution", "shared", "input.onSlowness", "wait");

    karabo::data::Hash config2(config1);
    config2.set<std::string>("deviceId", m_receiver2);

    instantiateDeviceWithAssert("PipeReceiverDevice", config1);
    instantiateDeviceWithAssert("PipeReceiverDevice", config2);

    testSenderOutputChannelConnections(2UL, {m_receiver1 + ":input", m_receiver2 + ":input"}, "shared", "wait", "local",
                                       {m_receiver1 + ":input2", m_receiver2 + ":input2"}, "copy", "drop", "local");

    testPipeTwoSharedReceivers(0, 0, 100, false, false);
    // The following line is commented out because:
    // 1. the result is not deterministic;
    // 2. segmentation fault has been observed, but rather rarely.
    // testPipeTwoSharedReceivers(200, 0, 0, false);
    // We expect to see data loss in the following cases:
    testPipeTwoSharedReceivers(100, 40, 0, true, false);  // receivers which have different "speed"
    testPipeTwoSharedReceivers(100, 100, 0, true, false); // receivers which have the same "speed"

    // Now test the shared input selector code, using round-robin
    m_deviceClient->set(m_sender, "nextSharedInput", "roundRobinSelector");
    testPipeTwoSharedReceivers(0, 0, 100, false, true);
    // If we expect data loss, we cannot be sure to have round-robin distribution
    testPipeTwoSharedReceivers(100, 40, 0, true, false);  // receivers which have different "speed"
    testPipeTwoSharedReceivers(100, 100, 0, true, false); // receivers which have the same "speed"

    // Reset shared input selector
    m_deviceClient->set(m_sender, "nextSharedInput", std::string());

    killDeviceWithAssert(m_receiver1);
    killDeviceWithAssert(m_receiver2);

    testSenderOutputChannelConnections();

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedReceiversQueue() {
    std::clog << "---\ntestPipeTwoSharedReceiversQueue (output.noInputShared = 'queueDrop', input.dataDistribution = "
                 "'shared')\n";

    const auto testStartTime = high_resolution_clock::now();

    // restart the sender with "output1.noInputShared == queueDrop"
    killDeviceWithAssert(m_sender);
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender, "output1.noInputShared", "queueDrop"));

    karabo::data::Hash config1(m_receiverBaseConfig);
    config1 += Hash("deviceId", m_receiver1, "input.dataDistribution", "shared");

    karabo::data::Hash config2(config1);
    config2.set<std::string>("deviceId", m_receiver2);

    instantiateDeviceWithAssert("PipeReceiverDevice", config1);
    instantiateDeviceWithAssert("PipeReceiverDevice", config2);

    testSenderOutputChannelConnections(2UL, {m_receiver1 + ":input", m_receiver2 + ":input"}, "shared", "drop", "local",
                                       {m_receiver1 + ":input2", m_receiver2 + ":input2"}, "copy", "drop", "local");

    // Set of tests for normal ('load-balanced') distribution mode.
    testPipeTwoSharedReceivers(0, 0, 100, false, false);
    // No data loss is expected for 'queueDrop' distribution mode, despite of differences between receivers
    // as long as there is no queue limit on the sender side and data fits into available chunks in OutputChannel
    testPipeTwoSharedReceivers(100, 40, 0, false, false);  // receivers which have different "speed"
    testPipeTwoSharedReceivers(100, 100, 0, false, false); // receivers which have the same "speed"

    testTwoSharedReceiversQueuing(5, 50);
    testTwoSharedReceiversQueuing(50, 5);

    // Now test the shared input selector code, using round-robin
    m_deviceClient->set(m_sender, "nextSharedInput", "roundRobinSelector");

    // Set of tests for 'round-robin' distribution mode.
    testPipeTwoSharedReceivers(0, 0, 20, false, true);
    // No data loss is expected for 'queueDrop' distribution mode, despite of differences between receivers
    // as long as there is no queue limit on the sender side and data fits into available chunks in OutputChannel
    testPipeTwoSharedReceivers(100, 40, 0, false, true);  // receivers which have different "speed"
    testPipeTwoSharedReceivers(100, 100, 0, false, true); // receivers which have the same "speed"

    testTwoSharedReceiversQueuing(5, 50);
    testTwoSharedReceiversQueuing(50, 5);

    // Reset shared input selector
    m_deviceClient->set(m_sender, "nextSharedInput", std::string());

    killDeviceWithAssert(m_receiver1);
    killDeviceWithAssert(m_receiver2);

    std::clog << "Test duration (ms): "
              << duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count() << std::endl;

    testSenderOutputChannelConnections();

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedReceivers(unsigned int processingTime1, unsigned int processingTime2,
                                                          unsigned int delayTime, bool dataLoss, bool roundRobin) {
    std::clog << "- processingTime1 = " << processingTime1 << " ms, processingTime2 = " << processingTime2
              << " ms, delayTime = " << delayTime << " ms" << (roundRobin ? " -- expect round-robin" : "") << "\n";

    m_deviceClient->set(m_receiver1, "processingTime", processingTime1);
    m_deviceClient->set(m_receiver2, "processingTime", processingTime2);
    m_deviceClient->set(m_sender, "delay", delayTime);

    // We use the same two receiver devices for several successive tests.
    // reset nTotalData and nTotalDataOnEos
    m_deviceClient->execute(m_receiver1, "reset");
    m_deviceClient->execute(m_receiver2, "reset");
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver1, "nTotalData", 0));
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver2, "nTotalData", 0));
    unsigned int nTotalData1 = 0;
    unsigned int nTotalData2 = 0;

    CPPUNIT_ASSERT_EQUAL(nTotalData1, m_deviceClient->get<unsigned int>(m_receiver1, "nTotalDataOnEos"));
    CPPUNIT_ASSERT_EQUAL(nTotalData2, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalDataOnEos"));
    for (unsigned int nRun = 0; nRun < m_numRunsPerTest; ++nRun) {
        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));
        // then call its slot
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        // poll until nTotalDataOnEos(s) of both receivers change (increase).
        // In case a load-balanced shared InputChannels, it is an implementation detail that both
        // receivers always get data - it is not logically required. If that detail changes,
        // one of the "nTotalDataOnEos" values could stay at its old value even if updated in an
        // EOS call and break this test here.
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver1, "nTotalDataOnEos", nTotalData1, false));
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver2, "nTotalDataOnEos", nTotalData2, false));

        const unsigned int nTotalData1New = m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData");
        const unsigned int nTotalData2New = m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData");

        // test nTotalDataOnEos == nTotalData
        CPPUNIT_ASSERT_EQUAL(nTotalData1New, m_deviceClient->get<unsigned int>(m_receiver1, "nTotalDataOnEos"));
        CPPUNIT_ASSERT_EQUAL(nTotalData2New, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalDataOnEos"));

        // test the total data received
        // A receiver should receive at least m_nPots data no matter how long the processingTime is.
        CPPUNIT_ASSERT(nTotalData1New >= nTotalData1 + m_nPots);
        CPPUNIT_ASSERT(nTotalData2New >= nTotalData2 + m_nPots);
        if (!dataLoss) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                  (boost::format("NoDataLoss assertion: expected: %d; actual: %d") %
                   (nTotalData1 + nTotalData2 + m_nDataPerRun) % (nTotalData1New + nTotalData2New))
                        .str(),
                  nTotalData1 + nTotalData2 + m_nDataPerRun, nTotalData1New + nTotalData2New);
        } else {
            CPPUNIT_ASSERT_MESSAGE((boost::format("DataLoss assertion: %d < %d") % (nTotalData1New + nTotalData2New) %
                                    (nTotalData1 + nTotalData2 + m_nDataPerRun))
                                         .str(),
                                   nTotalData1New + nTotalData2New < nTotalData1 + nTotalData2 + m_nDataPerRun);
        }

        // update nTotalData
        nTotalData1 = nTotalData1New;
        nTotalData2 = nTotalData2New;

        // test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string>>(m_receiver1, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1[0], sources[0]);
        auto sources2 = m_deviceClient->get<std::vector<std::string>>(m_receiver2, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1u), sources2.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1[0], sources2[0]);

        // check that receiver did not post any problem on status:
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver1, "status"));
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver2, "status"));

        if (roundRobin) {
            // Additional test that data share was fair, i.e. difference is zero for even total number or one for odd
            if ((nTotalData1New + nTotalData2New) % 2 == 0) { // even
                CPPUNIT_ASSERT_EQUAL(nTotalData1New, nTotalData2New);
            } else {
                const unsigned int diff = (nTotalData1New >= nTotalData2New        // unsigned, so cannot...
                                                 ? nTotalData1New - nTotalData2New // ...just calc the diff and take abs
                                                 : nTotalData2New - nTotalData1New);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                      (boost::format("total1: %d, total2: %d") % nTotalData1New % nTotalData2New).str(), 1u, diff);
            }
        }
    }

    std::clog << "  summary: nTotalData = " << nTotalData1 << ", " << nTotalData2 << std::endl;
}


void PipelinedProcessing_Test::testTwoSharedReceiversQueuing(unsigned int processingTime, unsigned int delayTime) {
    std::clog << "- processingTime (both receivers) = " << processingTime << " ms, delayTime = " << delayTime
              << " ms\n";

    int64_t elapsedTimeIn_microseconds = 0ll;

    // If processingTime is significantly bigger than delayTime, we are bound by processingTime. In this scenario,
    // the sender will start "sending" (actually dispatching data to a queue) immediately. As the receivers have a
    // relatively large processingTime, it will take a while for them to actually receive the data.
    bool processingTimeHigher = (processingTime > 2 * delayTime);

    // If delayTime is significantly bigger than the processing time, we are bound by the delayTime. This means
    // that between two successive data writes the sender will wait delayTime milliseconds and the sender is
    // expected to be slowest part.
    bool delayTimeHigher = (2 * processingTime < delayTime);

    CPPUNIT_ASSERT_MESSAGE("Difference between processingTime and delayTime not large enough to test queuing behavior!",
                           processingTimeHigher || delayTimeHigher);

    m_deviceClient->set(m_receiver1, "processingTime", processingTime);
    m_deviceClient->set(m_receiver2, "processingTime", processingTime);
    m_deviceClient->set(m_sender, "delay", delayTime);

    // We use the same two receiver devices for several successive tests.
    // reset nTotalData and nTotalDataOnEos
    m_deviceClient->execute(m_receiver1, "reset");
    m_deviceClient->execute(m_receiver2, "reset");
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver1, "nTotalData", 0));
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver2, "nTotalData", 0));

    unsigned int nDataExpected = 0;
    for (unsigned int nRun = 0; nRun < m_numRunsPerTest; ++nRun) {
        const auto startTimepoint = high_resolution_clock::now();

        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));
        // then call its slot again
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        // Makes sure the sender has finished sending the data in this run. We can't rely on 'currentDataId' for
        // this because in situations of high delayTime a pollDeviceProperty polling can return immediately due to
        // an expected value from the previous run.
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));

        nDataExpected += m_nDataPerRun;

        const unsigned int receivedSoFar1 = m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData");
        const unsigned int receivedSoFar2 = m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData");
        const unsigned int receivedSoFar = receivedSoFar1 + receivedSoFar2;

        if (processingTimeHigher) {
            // We assert a maximum ratio of data arrival in this scenario.
            CPPUNIT_ASSERT_MESSAGE(
                  karabo::data::toString(receivedSoFar) + " " + karabo::data::toString(nDataExpected),
                  2 * (nDataExpected - receivedSoFar) <= m_nDataPerRun * 3); // at max. 2/3 have arrived
        } else if (delayTimeHigher) {
            // We assert that the amount of received data must be at the maximum 2*m_nPots lower than the amount of
            // expected sent data (no bottleneck on the receivers).
            CPPUNIT_ASSERT_MESSAGE(karabo::data::toString(receivedSoFar) + " " + karabo::data::toString(nDataExpected),
                                   nDataExpected - receivedSoFar <= 2 * m_nPots);
        }

        // In the end, all data should have arrived - waits until the total amount of data received equals the total
        // sent (or fail).
        const int pollWaitTimeInMs = 5;
        int pollCounter = 0;
        bool allReceived = false;

        // Poll the device until it responds with the correct answer or times out.
        while (pollWaitTimeInMs * pollCounter <= m_maxTestTimeOut * 1000) {
            std::this_thread::sleep_for(milliseconds(pollWaitTimeInMs));
            const unsigned int received1 = m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData");
            const unsigned int received2 = m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData");
            const unsigned int received = received1 + received2;
            if (received == nDataExpected) {
                allReceived = true;
                break;
            }
            ++pollCounter;
        }
        CPPUNIT_ASSERT_MESSAGE("Unexpected data loss detected.", allReceived);

        const auto dur = high_resolution_clock::now() - startTimepoint;
        // Note that duration contains overhead from message travel time and polling interval in pollDeviceProperty!
        elapsedTimeIn_microseconds += duration_cast<microseconds>(dur).count();
    }

    const unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver1, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected) / double(elapsedTimeIn_microseconds);
    // Note that this measurement checks the inner-process shortcut - and includes timing overhead e.g.
    // pollDeviceProperty In addition, the process and delay times also affect mbps.
    std::clog << "  summary: Megabytes per sec : " << mbps
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds << ", dataItemSize = " << dataItemSize
              << ", nTotalData = " << nDataExpected << ", " << (processingTimeHigher ? "Queuing" : "No queuing")
              << " on sender detected, as expected." << std::endl;
}


void PipelinedProcessing_Test::testPipeTwoSharedReceiversQueueAtLimit() {
    // Here we test how the output to shared receivers behaves when running into the queue limit

    // Use common receiver devices - processing times can be reconfigured
    karabo::data::Hash config1(m_receiverBaseConfig);
    config1 += Hash("deviceId", m_receiver1, "input.dataDistribution", "shared");

    karabo::data::Hash config2(config1);
    config2.set<std::string>("deviceId", m_receiver2);

    instantiateDeviceWithAssert("PipeReceiverDevice", config1);
    instantiateDeviceWithAssert("PipeReceiverDevice", config2);

    // Subtests below restart m_sender (to configure its output channel). Cache here things that are set to non-default
    const int prev_delay = m_deviceClient->get<unsigned int>(m_sender, "delay");
    const int prev_nData = m_deviceClient->get<unsigned int>(m_sender, "nData");
    const int prev_dataSize = m_deviceClient->get<unsigned int>(m_sender, "dataSize");

    // 1) load-balanced
    // 1a) test slow receivers with sender queueDrop: drop data if queue gets full
    testPipeTwoSharedQueueDropAtLimit("load-balanced", 8, 7, 0, true, true); // dataLoss, slowReceivers
    // 1b) test fast receivers with sender queueDrop: do not drop data, since queue never full
    testPipeTwoSharedQueueDropAtLimit("load-balanced", 0, 1, 2, false, false); // dataLoss, slowReceivers

    // 2) round-robin, i.e. testing sharedInputSelector code
    // 2a) test slow receivers with sender queueDrop: drop data if queue gets full
    //     Seen failures with processingTimes 6/4 ...
    testPipeTwoSharedQueueDropAtLimit("round-robin", 13, 6, 0, true, true); // dataLoss, slowReceivers
    // 2b) test fast receivers with sender queueDrop: do not drop data, since queue never full
    testPipeTwoSharedQueueDropAtLimit("round-robin", 0, 1, 2, false, false); // dataLoss, slowReceivers

    killDeviceWithAssert(m_receiver1);
    killDeviceWithAssert(m_receiver2);

    testSenderOutputChannelConnections();

    m_deviceClient->set(m_sender, Hash("delay", prev_delay, "nData", prev_nData, "dataSize", prev_dataSize));

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedQueueDropAtLimit(const std::string& distributionMode,
                                                                 unsigned int processingTime1,
                                                                 unsigned int processingTime2, unsigned int senderDelay,
                                                                 bool expectDataLoss, bool slowReceivers) {
    std::clog << "---\ntestPipeTwoSharedQueueDropAtLimit: noInputShared = 'queueDrop', distributeQueue = '"
              << distributionMode << "', processing times " << processingTime1 << "/" << processingTime2
              << " ms, sender delay " << senderDelay << " ms\n";

    const bool roundRobin = (distributionMode == "round-robin"); // else load-balanced

    killDeviceWithAssert(m_sender);

    // We need a lot of data to fill up the queue so that data is indeed dropped
    const unsigned int nData = karabo::xms::Memory::MAX_N_CHUNKS + 1000u;
    const unsigned int dataSize = 1000u; // else memory trouble with big queues on small memory machines

    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender, "delay", senderDelay, "nData", nData,
                                                        "dataSize", dataSize, "output1.noInputShared", "queueDrop",
                                                        "nextSharedInput", (roundRobin ? "roundRobinSelector" : "")));

    m_deviceClient->set(m_receiver1, "processingTime", processingTime1);
    m_deviceClient->set(m_receiver2, "processingTime", processingTime2);

    testSenderOutputChannelConnections(2UL, {m_receiver1 + ":input", m_receiver2 + ":input"}, "shared", "drop", "local",
                                       {m_receiver1 + ":input2", m_receiver2 + ":input2"}, "copy", "drop", "local");

    const unsigned int nTotalDataStart1 = m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData");
    const unsigned int nTotalDataStart2 = m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData");

    CPPUNIT_ASSERT_EQUAL(nTotalDataStart1, m_deviceClient->get<unsigned int>(m_receiver1, "nTotalDataOnEos"));
    CPPUNIT_ASSERT_EQUAL(nTotalDataStart2, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalDataOnEos"));

    const auto testStartTime = high_resolution_clock::now();
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    // Makes sure the sender has finished sending the data in this run
    // (do not use waiting for sender state NORMAL - that is blocked by the call to EOS currently).
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_sender, "currentDataId", nData - 1u, true,
                                                    m_maxTestTimeOut * 4)); // Longer time out due to many data items

    const unsigned int receivedWhenWriteDone1 =
          m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData") - nTotalDataStart1;
    const unsigned int receivedWhenWriteDone2 =
          m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData") - nTotalDataStart2;

    const unsigned int missing = nData - (receivedWhenWriteDone1 + receivedWhenWriteDone2);
    if (slowReceivers) {
        // for sure not all arrived - missing more than what could still be in the pots of the 2 receiver buffers
        CPPUNIT_ASSERT_GREATER(2u * m_nPots, missing);
    } else {
        // No bottleneck on the receiver side: all is received, except what maybe sits in the 2 pots
        CPPUNIT_ASSERT_LESSEQUAL(2u * m_nPots, missing);
    }

    // poll until 'nTotalDataOnEos' of both receivers change (increase) because then all data is received
    // In case of load-balanced shared InputChannels, it is an implementation detail that both
    // receivers always get data - it is not logically required. If that detail changes,
    // one of the "nTotalDataOnEos" values could stay at its old value even if updated in an
    // EOS call and break this test here.
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver1, "nTotalDataOnEos", nTotalDataStart1, false));
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver2, "nTotalDataOnEos", nTotalDataStart2, false));

    const unsigned int nTotalEnd1 = m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData");
    const unsigned int nTotalEnd2 = m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData");
    // test nTotalDataOnEos == nTotalData
    CPPUNIT_ASSERT_EQUAL(nTotalEnd1, m_deviceClient->get<unsigned int>(m_receiver1, "nTotalDataOnEos"));
    CPPUNIT_ASSERT_EQUAL(nTotalEnd2, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalDataOnEos"));

    const unsigned int finallyReceived1 = nTotalEnd1 - nTotalDataStart1;
    const unsigned int finallyReceived2 = nTotalEnd2 - nTotalDataStart2;

    // A receiver should receive at least m_nPots data no matter how long the processingTime is.
    // (Note the comment above about the implementation detail for load-balanced...)
    CPPUNIT_ASSERT_GREATEREQUAL(m_nPots, finallyReceived1);
    CPPUNIT_ASSERT_GREATEREQUAL(m_nPots, finallyReceived2);

    if (slowReceivers) {
        // Since data was queued, now there has more arrived
        CPPUNIT_ASSERT_GREATEREQUAL(receivedWhenWriteDone1, finallyReceived1);
        CPPUNIT_ASSERT_GREATEREQUAL(receivedWhenWriteDone2, finallyReceived2);
    }

    if (roundRobin) {
        const unsigned int diff = (finallyReceived1 >= finallyReceived2        // unsigned, so cannot...
                                         ? finallyReceived1 - finallyReceived2 // ...just calc the diff and take abs
                                         : finallyReceived2 - finallyReceived1);
        if (expectDataLoss) {
            // If data loss, chunks might be skipped more often for the one receiver than for the other.
            // Failed here with 1%: https://git.xfel.eu/Karabo/Framework/-/jobs/141838
            // After refactoring to use async writing, the diff seems to be even larger, e.g. in
            // https://git.xfel.eu/Karabo/Framework/-/jobs/508636
            CPPUNIT_ASSERT_LESS(nData / 4u, diff); // arbitrarily tolerate 25% deviation of total number of items sent
        } else {
            // Additional test that data share was fair, i.e. difference is zero (one) for even (odd) total number
            if ((finallyReceived1 + finallyReceived2) % 2 == 0) { // even
                CPPUNIT_ASSERT_EQUAL(0u, diff);
            } else {
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                      (boost::format("total1: %d, total2: %d") % finallyReceived1 % finallyReceived2).str(), 1u, diff);
            }
        }
    }

    if (expectDataLoss) {
        // There is no CPPUNIT_ASSERT_LESS_MESSAGE...
        CPPUNIT_ASSERT_MESSAGE((boost::format("receiver 1: %d, receiver 2: %d, data sent %d") % finallyReceived1 %
                                finallyReceived2 % nData)
                                     .str(),
                               finallyReceived1 + finallyReceived2 < nData);
    } else {
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
              (boost::format("receiver 1: %d, receiver 2: %d") % finallyReceived1 % finallyReceived2).str(), nData,
              finallyReceived1 + finallyReceived2);
    }
    std::clog << "   Success - test duration "
              << duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count()
              << " ms: total data sent: " << nData << ", received when sent: " << receivedWhenWriteDone1 << "/"
              << receivedWhenWriteDone2 << ", received at the end: " << finallyReceived1 << "/" << finallyReceived2
              << std::endl;
}


void PipelinedProcessing_Test::testSharedReceiversSelector() {
    std::clog << "---\ntestSharedReceiversSelector\n";

    const Hash senderCfgBackup = m_deviceClient->get(m_sender);
    const unsigned int nData = 4;
    m_deviceClient->set(m_sender, Hash("nData", nData, "dataSize", 10u)); // decrease to ten to save energy

    // Check expectations from previous run
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_sender, "output1.noInputShared"));

    karabo::data::Hash config1(m_receiverBaseConfig);
    config1 += Hash("deviceId", m_receiver1, "input", Hash("dataDistribution", "shared", "onSlowness", "wait"));

    karabo::data::Hash config2(config1);
    config2.set<std::string>("deviceId", m_receiver2);

    instantiateDeviceWithAssert("PipeReceiverDevice", config1);
    instantiateDeviceWithAssert("PipeReceiverDevice", config2);

    // Ensure that connected
    testSenderOutputChannelConnections(2UL, {m_receiver1 + ":input", m_receiver2 + ":input"}, "shared", "wait", "local",
                                       {m_receiver1 + ":input2", m_receiver2 + ":input2"}, "copy", "drop", "local");

    // ==========================================================
    // Tell output channel to direct all data to receiver2
    m_deviceClient->set(m_sender, "nextSharedInput", m_receiver2 + ":input");
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    // Check that all data items arrived at receiver2 (and nothing at receiver1)
    pollDeviceProperty<unsigned int>(m_receiver2, "nTotalData", nData);
    CPPUNIT_ASSERT_EQUAL(nData, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData"));
    CPPUNIT_ASSERT_EQUAL(0u, m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData"));

    // ==========================================================
    // Tell output channel to direct all data to receiver1
    m_deviceClient->set(m_sender, "nextSharedInput", m_receiver1 + ":input");
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    // Check that all data arrived at receiver1 (and nothing at receiver2)
    pollDeviceProperty<unsigned int>(m_receiver1, "nTotalData", nData);
    CPPUNIT_ASSERT_EQUAL(nData, m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData"));
    CPPUNIT_ASSERT_EQUAL(
          nData, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData")); // as before, no increase (and no reset)

    // ==========================================================
    // Tell output channel to direct all data to something not connected (data will be dropped)
    m_deviceClient->set(m_sender, "nextSharedInput", "not_existing_device:input");
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    // Ensure sender is done and then sleep a bit, so any data would have had enough time to travel
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));
    std::this_thread::sleep_for(250ms);

    // No further new data has arrived at connected destinations
    CPPUNIT_ASSERT_EQUAL(nData, m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData"));
    CPPUNIT_ASSERT_EQUAL(nData, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData"));

    // ==========================================================
    // Tell output channel that desired destination is not connected (by making selector return empty string - data will
    // be dropped)
    m_deviceClient->set(m_sender, "nextSharedInput",
                        "returnEmptyString"); // magic value, see P2PSenderDevice::preReconfigure
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    // Ensure sender is done and then sleep a bit, so any data would have had enough time to travel
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));
    std::this_thread::sleep_for(250ms);

    // Also now: no further new data has arrived at connected destinations
    CPPUNIT_ASSERT_EQUAL(nData, m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData"));
    CPPUNIT_ASSERT_EQUAL(nData, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData"));

    // ==========================================================
    // Unregister selector from output channel - now all inputs are served again
    m_deviceClient->set(m_sender, "nextSharedInput", std::string());
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    // Check that both receivers are served again
    // Caveat: In case this falls back to "load-balanced", there is no 100% guarantee that both receive data.
    //         But if this ever fails, increasing m_sender.nData for this subtest should help.
    pollDeviceProperty<unsigned int>(m_receiver1, "nTotalData", nData, false); // not equals, so increased
    pollDeviceProperty<unsigned int>(m_receiver2, "nTotalData", nData, false); // dito
    CPPUNIT_ASSERT_GREATEREQUAL(nData + 1u, m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData"));
    CPPUNIT_ASSERT_GREATEREQUAL(nData + 1u, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData"));

    killDeviceWithAssert(m_receiver1);
    killDeviceWithAssert(m_receiver2);
    testSenderOutputChannelConnections();

    // Leave sender as found in the beginning
    m_deviceClient->set(m_sender, Hash("nData", senderCfgBackup.get<unsigned int>("nData"), //
                                       "dataSize", senderCfgBackup.get<unsigned int>("dataSize")));
    std::clog << "Passed!\n" << std::endl;
}


void PipelinedProcessing_Test::testQueueClearOnDisconnect() {
    std::clog << "---\ntestQueueClearOnDisconnect\n";

    const auto testStartTime = high_resolution_clock::now();

    testQueueClearOnDisconnectCopyQueue();

    // First with useRoundRobin true, last with false - so after test we have no remnant handler
    testQueueClearOnDisconnectSharedQueue(true);

    testQueueClearOnDisconnectSharedQueue(false);

    std::clog << "Test duration (ms): "
              << duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count() << std::endl;

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testQueueClearOnDisconnectSharedQueue(bool useRoundRobin) {
    std::clog << "- input.dataDistribution = 'shared', " << "output1.noInputShared = 'queueDrop', "
              << "sharedInputSelector: " << (useRoundRobin ? "round-robin" : "load-balanced") << std::endl;

    killDeviceWithAssert(m_sender);
    instantiateDeviceWithAssert("P2PSenderDevice",
                                Hash("deviceId", m_sender, "output1.noInputShared", "queueDrop", "nextSharedInput",
                                     (useRoundRobin ? "roundRobinSelector" : "")));

    // Instantiates the receiver with a really high processing time (in order of seconds) so that the sender won't
    // be able to send all the data before the receiver disconnects.
    karabo::data::Hash config(m_receiverBaseConfig);
    config.set("deviceId", m_receiver);
    config.set("input.dataDistribution", "shared");
    config.set("processingTime", 1000);
    instantiateDeviceWithAssert("PipeReceiverDevice", config);
    CPPUNIT_ASSERT_EQUAL(std::string("shared"), m_deviceClient->get<std::string>(m_receiver, "input.dataDistribution"));

    // Makes sure the sender is not sending any data before starting the test run.
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));
    // Assure that receiver is connected.
    // (Only checking "input.missingConnections" not 100% reliable, see InputChannel::onConnect.)
    testSenderOutputChannelConnections(1ul,                                                //
                                       {m_receiver + ":input"}, "shared", "drop", "local", // "drop": default
                                       {m_receiver + ":input2"}, "copy", "drop", "local"); // "copy" as well
    CPPUNIT_ASSERT(pollDeviceProperty(m_receiver, "input.missingConnections", std::vector<std::string>()));

    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    // Waits long enough for some data to arrive.
    pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", 0u, false);
    const unsigned int receivedBeforeDisc = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    // Checks that at least one data item has been received before the receiver entered in "processing" state.
    CPPUNIT_ASSERT(receivedBeforeDisc > 0); // Not redundant: the property polling might have timed out.

    const unsigned int nDataExpected = m_nDataPerRun;
    const unsigned int nDataFlushed = (m_nDataPerRun - receivedBeforeDisc);
    const unsigned int nDataReceived = receivedBeforeDisc;

    // Asserts that there's still data to be sent - data already received lower than expected data.
    CPPUNIT_ASSERT(3 * (nDataExpected - receivedBeforeDisc) > m_nDataPerRun * 2);

    // Disconnects the receiver by killing it.
    killDeviceWithAssert(m_receiver);

    // Check that sender has done its part and will not send anything after receiver is re-instantiated.
    // Otherwise it could be that not all data is put into queue and will be flushed as we want to test here.
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));

    // Re-instantiates the receiver - this time there's no need to use a high processingTime.
    karabo::data::Hash configAfterDisc(m_receiverBaseConfig);
    configAfterDisc.set("deviceId", m_receiver);
    configAfterDisc.set("input.dataDistribution", "shared");
    configAfterDisc.set("processingTime", 5);
    instantiateDeviceWithAssert("PipeReceiverDevice", configAfterDisc);
    CPPUNIT_ASSERT_EQUAL(std::string("shared"), m_deviceClient->get<std::string>(m_receiver, "input.dataDistribution"));
    // Assure that new incarnation of receiver is connected.
    // (Only checking "input.missingConnections" not 100% reliable, see InputChannel::onConnect.)
    testSenderOutputChannelConnections(1ul,                                                //
                                       {m_receiver + ":input"}, "shared", "drop", "local", // "drop": default
                                       {m_receiver + ":input2"}, "copy", "drop", "local"); // "copy" as well
    CPPUNIT_ASSERT(pollDeviceProperty(m_receiver, "input.missingConnections", std::vector<std::string>()));

    // Asserts that after a while (around 1 second), the receiver hasn't received any data - meaning that the queue
    // has been properly cleared after the receiver disconnected.
    std::this_thread::sleep_for(1000ms);
    const unsigned int receivedAfterReconnect = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    CPPUNIT_ASSERT_EQUAL(0u, receivedAfterReconnect);

    killDeviceWithAssert(m_receiver);

    // Prints summary data: total of data items sent, received and discarded
    std::clog << "  summary: data items to send = " << nDataExpected << ", data items received = " << nDataReceived
              << ", data items discarded = " << nDataFlushed << std::endl;
}


void PipelinedProcessing_Test::testQueueClearOnDisconnectCopyQueue() {
    std::clog << "- input.onSlowness = 'queueDrop', input.dataDistribution = 'copy'\n";

    unsigned int nDataExpected = 0, nDataReceived = 0, nDataFlushed = 0;

    // Instantiates the receiver with a really high processing time (in order of seconds) so that the sender won't
    // be able to send all the data before the receiver disconnects.
    karabo::data::Hash config(m_receiverBaseConfig);
    config.set("deviceId", m_receiver);
    config.set("input.onSlowness", "queueDrop");
    config.set("input.dataDistribution", "copy");
    config.set("processingTime", 1000);
    instantiateDeviceWithAssert("PipeReceiverDevice", config);
    CPPUNIT_ASSERT_EQUAL(std::string("queueDrop"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("copy"), m_deviceClient->get<std::string>(m_receiver, "input.dataDistribution"));
    // check for connection?

    // Makes sure the sender is not sending any data before starting the test run.
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));
    // Assure that receiver is connected.
    // (Only checking "input.missingConnections" not 100% reliable, see InputChannel::onConnect.)
    testSenderOutputChannelConnections(1ul,                                                   //
                                       {m_receiver + ":input"}, "copy", "queueDrop", "local", //
                                       {m_receiver + ":input2"}, "copy", "drop", "local");    // just defaults
    CPPUNIT_ASSERT(pollDeviceProperty(m_receiver, "input.missingConnections", std::vector<std::string>()));

    // call sender's slot
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    // Waits long enough for some data to arrive.
    pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", 0u, false);
    const unsigned int receivedBeforeDisc = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    // Checks that at least one data item has been received before the receiver entered in "processing" state.
    CPPUNIT_ASSERT(receivedBeforeDisc > 0); // Not redundant: the property polling might have timed out.

    nDataExpected += m_nDataPerRun;
    nDataFlushed += (m_nDataPerRun - receivedBeforeDisc);
    nDataReceived += receivedBeforeDisc;

    // Disconnects the receiver by killing it.
    killDeviceWithAssert(m_receiver);


    // Asserts that there's still data to be sent - data already received lower than expected data.
    CPPUNIT_ASSERT(3 * (nDataExpected - receivedBeforeDisc) > m_nDataPerRun * 2);

    // Re-instantiates the receiver - this time there's no need to use a high processingTime.
    karabo::data::Hash configAfterDisc(m_receiverBaseConfig);
    configAfterDisc.set("deviceId", m_receiver);
    configAfterDisc.set("input.onSlowness", "queueDrop");
    configAfterDisc.set("input.dataDistribution", "copy");
    configAfterDisc.set("processingTime", 5);
    instantiateDeviceWithAssert("PipeReceiverDevice", configAfterDisc);
    CPPUNIT_ASSERT_EQUAL(std::string("queueDrop"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("copy"), m_deviceClient->get<std::string>(m_receiver, "input.dataDistribution"));
    // Assure that receiver is connected.
    // (Only checking "input.missingConnections" not 100% reliable, see InputChannel::onConnect.)
    testSenderOutputChannelConnections(1ul,                                                   //
                                       {m_receiver + ":input"}, "copy", "queueDrop", "local", //
                                       {m_receiver + ":input2"}, "copy", "drop", "local");    // just defaults
    CPPUNIT_ASSERT(pollDeviceProperty(m_receiver, "input.missingConnections", std::vector<std::string>()));

    // Asserts that after a while (around 1 second), the receiver hasn't received any data - meaning that the queue
    // has been properly cleared after the receiver disconnected.
    std::this_thread::sleep_for(1000ms);
    const unsigned int receivedAfterReconnect = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    CPPUNIT_ASSERT_EQUAL(0u, receivedAfterReconnect);

    killDeviceWithAssert(m_receiver);

    // Prints summary data: total of data items sent, received and discarded
    std::clog << "  summary: data items to send = " << nDataExpected << ", data items received = " << nDataReceived
              << ", data items discarded = " << nDataFlushed << std::endl;
}


void PipelinedProcessing_Test::testProfileTransferTimes() {
    std::clog << "---\ntestProfileTransferTimes\n";

    const auto testStartTime = high_resolution_clock::now();
    // flags mean:      noShortCut, safeNDArray
    testProfileTransferTimes(false, false);
    testProfileTransferTimes(true, false);
    testProfileTransferTimes(false, true);
    testProfileTransferTimes(true, true);
    std::clog << "Test duration (ms): "
              << duration_cast<milliseconds>(high_resolution_clock::now() - testStartTime).count() << std::endl;

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testProfileTransferTimes(bool noShortCut, bool safeNDArray) {
    std::clog << "- (" << (noShortCut ? "no short cut" : "   short cut") << ", "
              << (safeNDArray ? "    safe ndarray" : "not safe ndarray") << "): " << std::flush;
    if (noShortCut) {
        setenv("KARABO_NO_PIPELINE_SHORTCUT", "1", 1);
    }
    // Looks like to get "KARABO_NO_PIPELINE_SHORTCUT" active (some caching?),
    // we have to re-instantiate the receiver.
    karabo::data::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "input2.onSlowness", "wait");
    instantiateDeviceWithAssert("PipeReceiverDevice", config);

    const unsigned int nDataPerRun = m_deviceClient->get<unsigned int>(m_sender, "nData");

    // set the scenario
    m_deviceClient->set(m_sender, "scenario", "profile");
    m_deviceClient->set(m_sender, "safeNDArray", safeNDArray);
    // make sure the sender has stopped sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::data::State>(m_sender, "state", karabo::data::State::NORMAL));
    // Assure that receiver is connected.
    // (Only checking "input.missingConnections" not 100% reliable, see InputChannel::onConnect.)
    const std::string memLoc(noShortCut ? "remote" : "local");                            // memory location
    testSenderOutputChannelConnections(1ul,                                               //
                                       {m_receiver + ":input"}, "copy", "drop", memLoc,   // "drop": default
                                       {m_receiver + ":input2"}, "copy", "wait", memLoc); // "copy" as well
    CPPUNIT_ASSERT(pollDeviceProperty(m_receiver, "input.missingConnections", std::vector<std::string>()));
    // Then call its slot
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

    // And poll for the correct answer
    const unsigned int expectedDataItems = nDataPerRun * 4; // sender sends 4 items per iteration
    pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", expectedDataItems);
    CPPUNIT_ASSERT_EQUAL(expectedDataItems, m_deviceClient->get<unsigned int>(m_receiver, "nTotalData"));

    pollDeviceProperty<float>(m_receiver, "averageTransferTime", 0.f, false); // until not zero anymore!
    float transferTime = m_deviceClient->get<float>(m_receiver, "averageTransferTime") / 1000;

    std::clog << transferTime << " milliseconds average transfer time" << std::endl;

    if (noShortCut) {
        unsetenv("KARABO_NO_PIPELINE_SHORTCUT");
    }
    killDeviceWithAssert(m_receiver);
}


template <typename T>
bool PipelinedProcessing_Test::pollDeviceProperty(const std::string& deviceId, const std::string& propertyName,
                                                  const T& expected, bool checkForEqual,
                                                  const int maxTimeoutInSec) const {
    const int pollWaitTimeInMs = 5;
    int pollCounter = 0;

    // Poll the device until it responds with the correct answer or times out.
    while (pollWaitTimeInMs * pollCounter <= maxTimeoutInSec * 1000) {
        std::this_thread::sleep_for(milliseconds(pollWaitTimeInMs));
        const T nReceived = m_deviceClient->get<T>(deviceId, propertyName);
        if ((checkForEqual && nReceived == expected) || (!checkForEqual && nReceived != expected)) {
            return true;
        }
        ++pollCounter;
    }

    return false;
}


void PipelinedProcessing_Test::instantiateDeviceWithAssert(const std::string& classId,
                                                           const karabo::data::Hash& configuration) {
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, classId, configuration, m_maxTestTimeOut);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
}


void PipelinedProcessing_Test::killDeviceWithAssert(const std::string& deviceId) {
    std::pair<bool, std::string> success = m_deviceClient->killDevice(deviceId, m_maxTestTimeOut);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
}
