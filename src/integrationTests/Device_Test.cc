/*
 * File:   Device_Test.cc
 * Author: gero.flucke@xfel.eu
 *
 */

#include "Device_Test.hh"

#include <karabo/core/Device.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/util/Epochstamp.hh>
#include <karabo/util/Timestamp.hh>

#define KRB_TEST_MAX_TIMEOUT 5

using karabo::net::EventLoop;
using karabo::util::Hash;
using karabo::util::Epochstamp;
using karabo::util::Timestamp;
using karabo::core::DeviceServer;
using karabo::core::DeviceClient;
using karabo::xms::SignalSlotable;

class TestDevice : public karabo::core::Device<> {

public:

    KARABO_CLASSINFO(TestDevice, "TestDevice", "2.1")

    static void expectedParameters(karabo::util::Schema& expected) {
    }


    TestDevice(const karabo::util::Hash& input) : karabo::core::Device<>(input) {
        // Bind to a slot what now is called from deviceServer:
        KARABO_SLOT(slotTimeTick, unsigned long long /*id*/, unsigned long long /*sec*/, unsigned long long /*frac*/, unsigned long long /*period*/)

        KARABO_SLOT(slotIdOfEpochstamp, unsigned long long /*sec*/, unsigned long long /*frac*/)
    }


    virtual ~TestDevice() {
    }


    void slotIdOfEpochstamp(unsigned long long sec, unsigned long long frac) {
        const Timestamp stamp(getTimestamp(Epochstamp(sec, frac)));
        reply(stamp.getTrainId());
    }

};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, TestDevice)


CPPUNIT_TEST_SUITE_REGISTRATION(Device_Test);


Device_Test::Device_Test() {
}


Device_Test::~Device_Test() {
}


void Device_Test::setUp() {
    // uncomment this if ever testing against a local broker
    //setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));
    // Create and start server
    {
        Hash config("serverId", "testServerDevice", "scanPlugins", false, "Logger.priority", "FATAL");
        m_deviceServer = DeviceServer::create("DeviceServer", config);
        m_deviceServer->finalizeInternalInitialization();
    }

    // Create client
    m_deviceClient = boost::make_shared<DeviceClient>();

}


void Device_Test::tearDown() {
    m_deviceServer.reset();
    m_deviceClient.reset();

    EventLoop::stop();
    m_eventLoopThread.join();
}


void Device_Test::appTestRunner() {

    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerDevice", "TestDevice",
                                                                       Hash("deviceId", "TestDevice"),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Now all possible individual tests - one only for now
    testGetTimestamp();
}


void Device_Test::testGetTimestamp() {
    // This tests the extrapolations done in Device::getTimestamp(Epochstamp& epoch)
    // and Device::slotGetTime().

    // Setup a communication helper
    auto sigSlotA = boost::make_shared<SignalSlotable>("sigSlotA");
    sigSlotA->start();

    const int timeOutInMs = 250;
    const unsigned long long periodInMicroSec = 100000ull; // some tests below assume this to be 0.1 s
    const unsigned long long periodInAttoSec = periodInMicroSec * 1000000000000ull;
    // Before first received time tick, always return train id 0
    unsigned long long id = 1111ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              1ull, 2ull) // values here should not matter at all
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(0ull, id);

    // Also slotGetTime has zero train id
    Epochstamp now;
    Hash timeHash;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotGetTime").timeout(timeOutInMs).receive(timeHash));
    CPPUNIT_ASSERT(timeHash.has("time"));
    CPPUNIT_ASSERT(timeHash.get<bool>("time"));
    const Timestamp stamp(Timestamp::fromHashAttributes(timeHash.getAttributes("time")));
    CPPUNIT_ASSERT_EQUAL(0ull, stamp.getTrainId());
    CPPUNIT_ASSERT(stamp.getEpochstamp() > now);

    // Now send a time tick...
    const unsigned long long seconds = 1559600000ull; // About June 3rd, 2019, 10 pm GMT
    const unsigned long long startId = 100ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotTimeTick",
                                              // id,     sec,    frac (attosec), period (microsec)
                                              startId, seconds, 2ull * periodInAttoSec + 1100ull, periodInMicroSec)
                            .timeout(timeOutInMs).receive());

    timeHash.clear();
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotGetTime").timeout(timeOutInMs).receive(timeHash));
    const Timestamp stamp2(Timestamp::fromHashAttributes(timeHash.getAttributes("time")));
    CPPUNIT_ASSERT_GREATEREQUAL(startId, stamp2.getTrainId());

    // ...and test real calculations of id
    // 1) exact match
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds, 2ull * periodInAttoSec + 1100ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId, id);

    // 2) end of id
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds, 3ull * periodInAttoSec + 1099ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId, id);

    // 3) multiple of period above - but same second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds, 5ull * periodInAttoSec + 1100ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId + 3ull, id);

    // 4) multiple of period plus a bit above - next second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds + 1ull, 5ull * periodInAttoSec + 1105ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId + 13ull, id);

    // 5) just before
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds, 2ull * periodInAttoSec + 1090ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId - 1ull, id);

    // 6) several before - but same second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds, 1ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId - 3ull, id);

    // 7) several before - previous second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds - 1ull, 5ull * periodInAttoSec + 1110ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId - 7ull, id);

    // 8) so much in the past that a negative id would be calculated which leads to zero
    id = 1111ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds - 100ull, 1110ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(0ull, id);
}
