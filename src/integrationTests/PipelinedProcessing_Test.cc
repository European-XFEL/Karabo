/*
 * File:   PipelineProcessing_Test.cc
 * Author: haufs
 *
 * Modified by J. Zhu
 *
 * Created on Sep 20, 2016, 3:40:34 PM
 */

#include "PipelinedProcessing_Test.hh"
#include <karabo/net/EventLoop.hh>
#include <karabo/log/Logger.hh>

#include <boost/format.hpp>

USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(PipelinedProcessing_Test);


PipelinedProcessing_Test::PipelinedProcessing_Test() = default;


PipelinedProcessing_Test::~PipelinedProcessing_Test() = default;


void PipelinedProcessing_Test::setUp() {

    // set broker
    //putenv("KARABO_BROKER=tcp://localhost:7777");

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));

    // Create and start server
    Hash config("serverId", m_server, "scanPlugins", false, "Logger.priority", "ERROR");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();

    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());
}


void PipelinedProcessing_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();

    EventLoop::stop();
    m_eventLoopThread.join();
}


void PipelinedProcessing_Test::appTestRunner() {
    // in order to avoid recurring setup and tear down calls, all tests are run in a single runner
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender));
    m_nDataPerRun = m_deviceClient->get<unsigned int>(m_sender, "nData");

    testGetOutputChannelSchema();

    testPipeWait();

    testPipeDrop();

    testPipeQueue();

    testPipeMinData();

    testPipeTwoPots();

    // Now order of sub-tests starts to matter:
    // After this test, the sender will have "output1.distributionMode == round-robin".
    testPipeTwoSharedReceiversWait();

    // Test does not care about "output1.distributionMode == round-robin",
    // but after test it will be back to load-balanced and have "output1.noInputShared == drop".
    testPipeTwoSharedReceiversDrop();

    testPipeTwoSharedReceiversQueue();

    // this test uses output2 channel of the sender
    testProfileTransferTimes();

    killDeviceWithAssert(m_sender);
}


void PipelinedProcessing_Test::testGetOutputChannelSchema() {
    std::clog << "---\ntestGetOutputChannelSchema\n";

    karabo::util::Hash dataSchema = m_deviceClient->getOutputChannelSchema(m_sender, "output1");

//    clog << "\nPipelinedProcessing_Test::testGetOutputChannelSchema() : dataSchema => \n" << dataSchema << endl;

    CPPUNIT_ASSERT(dataSchema.has("dataId"));
    CPPUNIT_ASSERT(dataSchema.getType("dataId") == karabo::util::Types::INT32);
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("dataId", "valueType") == "INT32");
    CPPUNIT_ASSERT(dataSchema.has("sha1"));
    CPPUNIT_ASSERT(dataSchema.getType("sha1") == karabo::util::Types::INT32);
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("sha1", "valueType") == "STRING");
    CPPUNIT_ASSERT(dataSchema.has("data"));
    CPPUNIT_ASSERT(dataSchema.getType("data") == karabo::util::Types::INT32);
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("data", "valueType") == "VECTOR_INT64");
    CPPUNIT_ASSERT(dataSchema.has("array"));
    CPPUNIT_ASSERT(dataSchema.hasAttribute("array", KARABO_HASH_CLASS_ID));
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array", KARABO_HASH_CLASS_ID) == "Hash");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array", "classId") == "NDArray");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.data", "valueType") == "BYTE_ARRAY");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.shape", "valueType") == "VECTOR_UINT64");
    CPPUNIT_ASSERT(dataSchema.getAttributeAs<std::string>("array.shape", "defaultValue") == "100,200,0");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.type", "valueType") == "INT32");
    CPPUNIT_ASSERT(dataSchema.getAttributeAs<std::string>("array.type", "defaultValue") == "22");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.isBigEndian", "valueType") == "BOOL");
    CPPUNIT_ASSERT(dataSchema.getAttributeAs<std::string>("array.isBigEndian", "defaultValue") == "0");

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeWait() {
    std::clog << "---\ntestPipeWait (onSlowness = 'wait')\n";
    // use only one receiver for a group of tests
    karabo::util::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver);
    instantiateDeviceWithAssert("PipeReceiverDevice", config);
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));

    testPipeWait(0, 0);
    testPipeWait(100, 0);
    testPipeWait(0, 100);

    killDeviceWithAssert(m_receiver);
    std::clog << "Passed!\n\n";
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
        const auto startTimepoint = std::chrono::high_resolution_clock::now();
        
        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));
        // Then call its slot
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        nDataExpected += m_nDataPerRun;
        // And poll for the correct answer
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataExpected));

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        // Note that duration contains overhead from message travel time and polling interval in pollDeviceProperty!
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        // Check that EOS handling is not called too early
        
        // EOS comes a bit later, so we have to poll client again to be sure...
        // Note: only one EOS arrives after receiving a train of data!
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected));

        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources[0]);
        // Check that receiver did not post any problem on status:
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver, "status"));
        // This only can be tested if we used an input handler and not onData
        if (!m_deviceClient->get<bool>(m_receiver, "onData")) {
            auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSourcesFromIndex");
            CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources[0]);
        }
    }

    const unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    // Note that this measurement checks the inner-process shortcut - and includes timing overhead e.g. pollDeviceProperty
    // In addition, the process and delay times also affect mbps.
    std::clog << "  summary: Megabytes per sec : " << mbps
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
              << ", dataItemSize = " << dataItemSize 
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl;
}


void PipelinedProcessing_Test::testPipeDrop() {
    std::clog << "---\ntestPipeDrop (onSlowness = 'drop')\n";

    karabo::util::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "input.onSlowness", "drop");
    instantiateDeviceWithAssert("PipeReceiverDevice", config);
    CPPUNIT_ASSERT_EQUAL(std::string("drop"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));

    testPipeDrop(10, 0, true);
    testPipeDrop(100, 0, true);
    testPipeDrop(0, 100, false);

    killDeviceWithAssert(m_receiver);
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
        auto startTimepoint = std::chrono::high_resolution_clock::now();
        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));
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

        auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        // test EOS
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected));

        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources[0]);
    }

    unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    std::clog << "  summary: Megabytes per sec : " << mbps
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
              << ", dataItemSize = " << dataItemSize 
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl;
}

void PipelinedProcessing_Test::testPipeQueue() {
    std::clog << "---\ntestPipeQueue (onSlowness = 'queue', dataDistribution = 'copy')\n";

    karabo::util::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "input.onSlowness", "queue");
    config += Hash("deviceId", m_receiver, "input.dataDistribution", "copy");
    instantiateDeviceWithAssert("PipeReceiverDevice", config);
    CPPUNIT_ASSERT_EQUAL(std::string("queue"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("copy"), m_deviceClient->get<std::string>(m_receiver, "input.dataDistribution"));

    // Higher processing times are used to allow observation that data is sent faster than it is 
    // handled by the receiver.
    testPipeQueue(50, 5);
    // Higher delay times are used to allow observation that the sender becomes the bottleneck in
    // those scenarios.
    testPipeQueue(5, 50);

    killDeviceWithAssert(m_receiver);
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
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));

        // Retrieves the amount of data that the sender will send
        unsigned int senderNData = m_deviceClient->get<unsigned int>(m_sender, "nData");
        // Then call its slot
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        // Makes sure the sender has finished sending the data in this run. We can't rely on 'currentDataId' for
        // this because in situations of high delayTime a pollDeviceProperty polling can return immediately due to
        // an expected value from the previous run.
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));

        nDataExpected += m_nDataPerRun;
        if (processingTime > 2 * delayTime) {
            // If processingTime is significantly bigger than delayTime, we are bound by processingTime. In this scena-
            // rio, the sender will start "sending" (actually dispatching data to a queue) immediately. As the receiver
            // has a relatively large processingTime, it will take a while for the receiver to actually receive the data.
            // We assert a maximum ratio of data arrival in this scenario.
            const unsigned int receivedSoFar = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
            CPPUNIT_ASSERT_MESSAGE(karabo::util::toString(receivedSoFar) + " " + karabo::util::toString(nDataExpected),
                                   2 * (nDataExpected - receivedSoFar) <= m_nDataPerRun * 3);// at max. 2/3 have arrived
        } else if (2 * processingTime < delayTime) {
            // If delayTime is significantly bigger than the processing time, we are bound by the delayTime. This means
            // that between two successive data writes the sender will wait delayTime milliseconds and the sender is
            // expected to be slowest part.
            // We assert that the amount of received data must be at the maximum m_nPots lower than the amount of 
            // expected sent data (no bottleneck on the receiver).
            const unsigned int receivedSoFar = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
            CPPUNIT_ASSERT_MESSAGE(karabo::util::toString(receivedSoFar) + " " + karabo::util::toString(nDataExpected),
                                   nDataExpected - receivedSoFar <= m_nPots);
        }
        // In the end, all should arrive
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataExpected));

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        // Note that duration contains overhead from message travel time and polling interval in pollDeviceProperty!
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        // The following line is commented out because when 'onSlowness' is set to 'queue' the EOS may
        // be received and handled before data that comes before EOS. This is a pending issue and this
        // assert should be uncommented as part of the validation of a fix for that issue. For 'wait'
        // value for the 'onSlowness' setting the issue doesn't happen.
        // TODO!
        //CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected));

        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources[0]);
        // Check that receiver did not post any problem on status:
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver, "status"));
        // This only can be tested if we used an input handler and not onData
        if (!m_deviceClient->get<bool>(m_receiver, "onData")) {
            auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSourcesFromIndex");
            CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources[0]);
        }
    }

    const unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    // Note that this measurement checks the inner-process shortcut - and includes timing overhead e.g. pollDeviceProperty
    // In addition, the process and delay times also affect mbps.
    std::clog << "  summary: Megabytes per sec : " << mbps
            << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
            << ", dataItemSize = " << dataItemSize
            << ", nTotalData = " << nDataExpected - nTotalData0
            << ", nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl;
}


void PipelinedProcessing_Test::testPipeMinData() {
    std::clog << "---\ntestPipeWaitOnData\n";

    // input.minData = 1 by default
    unsigned int minData = 5;

    // start a receiver with "input.onData = false", i.e. call PipeReceiverDevice::onInput while reading data,
    // and "minData > 1"
    karabo::util::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "input.minData", minData);
    instantiateDeviceWithAssert("PipeReceiverDevice", config);

    // make sure the sender has stopped sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));

    // write data asynchronously
    m_deviceClient->execute(m_sender, "write");

    // poll until nTotalDataOnEos changes
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", 0, false));

    // test if data source was correctly passed
    auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSourcesFromIndex");
    // test that "input.onData = false" and "input.miniData" are respected
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(minData), sources.size());
    for (auto& src : sources) {
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1, src);
    }

    // in this case, only 10 data out of 12 are expected to be received
    unsigned int nDataExpected = m_nDataPerRun - m_nDataPerRun % minData;
    CPPUNIT_ASSERT_EQUAL(nDataExpected, m_deviceClient->get<unsigned int>(m_receiver, "nTotalData"));

    killDeviceWithAssert(m_receiver);
    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoPots() {
    std::clog << "---\ntestTwoPots\n";

    // start a receiver whose processingTime is significantly longer than the writing time of the output channel
    karabo::util::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver, "processingTime", 200);
    instantiateDeviceWithAssert("PipeReceiverDevice", config);

    for (unsigned int nDataWhenStop = 3; nDataWhenStop < 8; ++nDataWhenStop) {
        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));

        // write data asynchronously
        m_deviceClient->execute(m_sender, "write");

        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataWhenStop));
        // stop sending data after receiving nDataWhenStop data!
        m_deviceClient->execute(m_sender, "stop");
        // The receiver is expected to get one more data when EOS arrives: the one which is being written
        // into the inactive pot when the "stop" slot is called.
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", 0, false));
        CPPUNIT_ASSERT_EQUAL(nDataWhenStop + 1, m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos"));
        
        // reset nTotalData and nTotalDataOnEos
        m_deviceClient->execute(m_receiver, "reset");
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", 0));
    }

    killDeviceWithAssert(m_receiver);
    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedReceiversWait() {
    std::clog << "---\ntestPipeTwoSharedReceiversWait (onSlowness = 'wait', dataDistribution = 'shared')\n";

    karabo::util::Hash config1(m_receiverBaseConfig);
    config1 += Hash("deviceId", m_receiver1, "input.onSlowness", "wait", "input.dataDistribution", "shared");

    karabo::util::Hash config2(config1);
    config2.set<std::string>("deviceId", m_receiver2);

    instantiateDeviceWithAssert("PipeReceiverDevice", config1);
    instantiateDeviceWithAssert("PipeReceiverDevice", config2);

    // check that the default value of dataDistribution is "copy"
    CPPUNIT_ASSERT_EQUAL(std::string("copy"), m_deviceClient->get<std::string>(m_receiver1, "input2.dataDistribution"));
    // check that the default value of noInputShared is "wait"
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_sender, "output1.noInputShared"));

    testPipeTwoSharedReceivers(0, 0, 0, false, false);
    testPipeTwoSharedReceivers(200, 0, 0, false, false);
    testPipeTwoSharedReceivers(100, 100, 0, false, false);

    // restart the sender with "output1.distributionMode == round-robin" (and default "output1.noInputShared == wait")
    killDeviceWithAssert(m_sender);
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender, "output1", Hash("distributionMode", "round-robin")));

    testPipeTwoSharedReceivers(0, 0, 20, false, true);
    testPipeTwoSharedReceivers(200, 0, 0, false, true);
    testPipeTwoSharedReceivers(100, 100, 0, false, true);
    
    killDeviceWithAssert(m_receiver1);
    killDeviceWithAssert(m_receiver2);
    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedReceiversDrop() {
    std::clog << "---\ntestPipeTwoSharedReceiversDrop (onSlowness = 'drop', dataDistribution = 'shared')\n";

    karabo::util::Hash config1(m_receiverBaseConfig);
    config1 += Hash("deviceId", m_receiver1, "input.onSlowness", "drop", "input.dataDistribution", "shared");

    karabo::util::Hash config2(config1);
    config2.set<std::string>("deviceId", m_receiver2);

    instantiateDeviceWithAssert("PipeReceiverDevice", config1);
    instantiateDeviceWithAssert("PipeReceiverDevice", config2);

    // check that even if the receiver has "onSlowness == drop", all data will still be received by shared
    // receivers if "onInputShared" from the output channel of the sender is "wait"!
    // Do not care about "output1.distributionMode == round-robin" or " == load-balanced"
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_sender, "output1.noInputShared"));
    testPipeTwoSharedReceivers(100, 100, 0, false, false);

    // restart the sender with "output1.noInputShared == drop"
    killDeviceWithAssert(m_sender);
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender, "output1.noInputShared", "drop"));

    // check that the default value of distributionMode is "load-balanced"
    CPPUNIT_ASSERT_EQUAL(std::string("load-balanced"), m_deviceClient->get<std::string>(m_sender, "output1.distributionMode"));

    testPipeTwoSharedReceivers(0, 0, 100, false, false);
    // The following line is commented out because:
    // 1. the result is not deterministic;
    // 2. segmentation fault has been observed, but rather rarely.
    // testPipeTwoSharedReceivers(200, 0, 0, false);
    // We expect to see data loss in the following cases:
    testPipeTwoSharedReceivers(100, 40, 0, true, false); // receivers which have different "speed"
    testPipeTwoSharedReceivers(100, 100, 0, true, false); // receivers which have the same "speed"

    // restart the sender with "output1.noInputShared == drop" and "output1.distributionMode = round-robin"
    killDeviceWithAssert(m_sender);
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender, "output1", Hash("noInputShared", "drop",
                                                                                              "distributionMode", "round-robin")));

    testPipeTwoSharedReceivers(0, 0, 20, false, true);
    testPipeTwoSharedReceivers(100, 40, 0, true, true); // receivers which have different "speed"
    testPipeTwoSharedReceivers(100, 100, 0, true, true); // receivers which have the same "speed"

    killDeviceWithAssert(m_receiver1);
    killDeviceWithAssert(m_receiver2);

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedReceiversQueue() {
    std::clog << "---\ntestPipeTwoSharedReceiversQueue (onSlowness = 'queue', dataDistribution = 'shared')\n";

    karabo::util::Hash config1(m_receiverBaseConfig);
    config1 += Hash("deviceId", m_receiver1, "input.onSlowness", "queue", "input.dataDistribution", "shared");

    karabo::util::Hash config2(config1);
    config2.set<std::string>("deviceId", m_receiver2);

    instantiateDeviceWithAssert("PipeReceiverDevice", config1);
    instantiateDeviceWithAssert("PipeReceiverDevice", config2);

    // restart the sender with "output1.noInputShared == queue"
    killDeviceWithAssert(m_sender);
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender, "output1.noInputShared", "queue"));

    // check that the default value of distributionMode is "load-balanced"
    CPPUNIT_ASSERT_EQUAL(std::string("load-balanced"), m_deviceClient->get<std::string>(m_sender, "output1.distributionMode"));

    // Set of tests for 'load-balanced' distribution mode.
    testPipeTwoSharedReceivers(0, 0, 100, false, false);
    // No data loss is expected for 'queue' distribution mode, despite of differences between receivers
    testPipeTwoSharedReceivers(100, 40, 0, false, false); // receivers which have different "speed"
    testPipeTwoSharedReceivers(100, 100, 0, false, false); // receivers which have the same "speed"

    // restart the sender with "output1.noInputShared == queue" and "output1.distributionMode = "round-robin"
    killDeviceWithAssert(m_sender);
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender, "output1", Hash("noInputShared", "queue",
                                                                                              "distributionMode", "round-robin")));

    // Set of tests for 'round-robin' distribution mode.
    testPipeTwoSharedReceivers(0, 0, 20, false, true);
    // No data loss is expected for 'queue' distribution mode, despite of differences between receivers
    testPipeTwoSharedReceivers(100, 40, 0, false, true); // receivers which have different "speed"
    testPipeTwoSharedReceivers(100, 100, 0, false, true); // receivers which have the same "speed"

    killDeviceWithAssert(m_receiver1);
    killDeviceWithAssert(m_receiver2);

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedReceivers(unsigned int processingTime1, 
                                                          unsigned int processingTime2,
                                                          unsigned int delayTime,
                                                          bool dataLoss, bool roundRobin) {

    std::clog << "- processingTime1 = " << processingTime1
              << " ms, processingTime2 = " << processingTime2
            << " ms, delayTime = " << delayTime << " ms -- expect " << (roundRobin ? "round-robin" : "load-balanced") << "\n";

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
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));
        // then call its slot
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        // poll until nTotalDataOnEos(s) of both receivers change (increase)
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver1, "nTotalDataOnEos", nTotalData1, false));
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver2, "nTotalDataOnEos", nTotalData2, false));

        const unsigned int nTotalData1New = m_deviceClient->get<unsigned int>(m_receiver1, "nTotalData");
        const unsigned int nTotalData2New = m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData");

        // test nTotalDataOnEos == nTotalData
        CPPUNIT_ASSERT_EQUAL(nTotalData1New, m_deviceClient->get<unsigned int>(m_receiver1, "nTotalDataOnEos"));
        CPPUNIT_ASSERT_EQUAL(nTotalData2New, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalDataOnEos"));

        // test the total data received
        // A receiver should receive at least m_nPots data no mater how long the processingTime is.
        CPPUNIT_ASSERT(nTotalData1New >= nTotalData1 + m_nPots);
        CPPUNIT_ASSERT(nTotalData2New >= nTotalData2 + m_nPots);
        if (!dataLoss) {
            CPPUNIT_ASSERT_EQUAL(nTotalData1 + nTotalData2 + m_nDataPerRun, nTotalData1New + nTotalData2New);
        } else {
            CPPUNIT_ASSERT(nTotalData1New + nTotalData2New < nTotalData1 + nTotalData2 + m_nDataPerRun);
        }

        // update nTotalData
        nTotalData1 = nTotalData1New;
        nTotalData2 = nTotalData2New;

        // test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver1, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources[0]);
        auto sources2 = m_deviceClient->get<std::vector<std::string> >(m_receiver2, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources2.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources2[0]);

        // check that receiver did not post any problem on status:
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver1, "status"));
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver2, "status"));

        if (roundRobin) {
            // Additional test that data share was fair, i.e. difference is zero for even total number or one for odd
            if ((nTotalData1New + nTotalData2New) % 2 == 0) { // even
                CPPUNIT_ASSERT_EQUAL(nTotalData1New, nTotalData2New);
            } else {
                const unsigned int diff = (nTotalData1New >= nTotalData2New // unsigned, so cannot...
                                           ? nTotalData1New - nTotalData2New // ...just calc the diff and take abs
                                           : nTotalData2New - nTotalData1New);
                CPPUNIT_ASSERT_EQUAL_MESSAGE((boost::format("total1: %d, total2: %d") % nTotalData1New % nTotalData2New).str(),
                                             1u, diff);
            }
        }
    }

    std::clog << "  summary: nTotalData = " << nTotalData1 << ", " << nTotalData2 << std::endl;
}


void PipelinedProcessing_Test::testProfileTransferTimes() {
    std::clog << "---\ntestProfileTransferTimes\n";

    testProfileTransferTimes(false, true);
    testProfileTransferTimes(false, false);
    testProfileTransferTimes(true, true);
    testProfileTransferTimes(true, false);

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testProfileTransferTimes(bool noShortCut, bool copy) {

    if (noShortCut) {
        setenv("KARABO_NO_PIPELINE_SHORTCUT", "1", 1);
    }
    // Looks like to get "KARABO_NO_PIPELINE_SHORTCUT" active (some caching?),
    // we have to re-instantiate the receiver.
    karabo::util::Hash config(m_receiverBaseConfig);
    config += Hash("deviceId", m_receiver);
    instantiateDeviceWithAssert("PipeReceiverDevice", config);

    const unsigned int nDataPerRun = m_deviceClient->get<unsigned int>(m_sender, "nData");

    // set the scenario
    m_deviceClient->set(m_sender, "scenario", "profile");
    m_deviceClient->set(m_sender, "copyAllData", copy);
    // make sure the sender has stopped sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));
    // Then call its slot
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);
    
    // And poll for the correct answer
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataPerRun));

    pollDeviceProperty<float>(m_receiver, "averageTransferTime", 0.f, false); // until not zero anymore!
    float transferTime = m_deviceClient->get<float>(m_receiver, "averageTransferTime") / 1000;

    std::clog << "- ("
              << (copy ? "copy" : "no copy") << ", " << (noShortCut ? "no short cut" : "short cut")
              << "): " << transferTime << " milliseconds average transfer time" << std::endl;

    if (noShortCut) {
        unsetenv("KARABO_NO_PIPELINE_SHORTCUT");
    }
    killDeviceWithAssert(m_receiver);
}


template <typename T>
bool PipelinedProcessing_Test::pollDeviceProperty(const std::string& deviceId,
                                                  const std::string& propertyName, const T& expected, bool checkForEqual, 
                                                  const int maxTimeoutInSec) const {

    const int pollWaitTimeInMs = 5;
    int pollCounter = 0;

    // Poll the device until it responds with the correct answer or times out.
    while (pollWaitTimeInMs * pollCounter <= maxTimeoutInSec * 1000) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(pollWaitTimeInMs));
        const T nReceived = m_deviceClient->get<T>(deviceId, propertyName);
        if ((checkForEqual && nReceived == expected) || (!checkForEqual && nReceived != expected)) {
            return true;
        }
        ++pollCounter;
    }

    return false;
}


void PipelinedProcessing_Test::instantiateDeviceWithAssert(const std::string& classId,
                                                           const karabo::util::Hash& configuration) {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, classId, configuration, m_maxTestTimeOut);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
}


void PipelinedProcessing_Test::killDeviceWithAssert(const std::string& deviceId) {
    std::pair<bool, std::string> success = m_deviceClient->killDevice(deviceId, m_maxTestTimeOut);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
}
