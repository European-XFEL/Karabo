/*
 * File:   Timing_Test.cc
 * Author: steffen.hauf@xfel.eu
 
 */

#include "Timing_Test.hh"
#include <karabo/net/EventLoop.hh>
#include <karabo/util/StringTools.hh>
#include <cstdlib>


using namespace std;

#define KRB_TEST_MAX_TIMEOUT 5

USING_KARABO_NAMESPACES

CPPUNIT_TEST_SUITE_REGISTRATION(Timing_Test);


Timing_Test::Timing_Test() {
}


Timing_Test::~Timing_Test() {
}


void Timing_Test::setUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));
    // Create and start server
    Hash config("serverId", "testServerTiming", "scanPlugins", false, "Logger.priority", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());

}


void Timing_Test::tearDown() {
    m_deviceServer.reset();
    EventLoop::stop();
    m_eventLoopThread.join();
}


void Timing_Test::appTestRunner() {

    // Bring up a (simulated) time server and a time testing device
    const unsigned long long tickPeriodInMicrosec = 50000ull; // 50 ms
    const long long tickCountdown = 20u; // i.e. every 20th id is published
    const float periodVarFrac = 0.1f; // i.e. sometimes the published period is off by 10% up or down
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerTiming", "SimulatedTimeServerDevice",
                                                                       Hash("deviceId", "Karabo_TimeServer",
                                                                            "period", tickPeriodInMicrosec,
                                                                            "tickCountdown", tickCountdown,
                                                                            "periodVariationFraction", periodVarFrac),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    success = m_deviceClient->instantiate("testServerTiming", "TimingTestDevice", Hash("deviceId", "timeTester", "useTimeserver", true),
                                          KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    m_lastCheck = karabo::util::Epochstamp();

    // Give some time to connect the timing slot.
    // If this fails, it is likely that the signal connection was erased again because the call to slotInstanceNew
    // of the Karabo_TimeServer came late and cleared the signal.
    int counter = 0;
    while (true) {
        if (m_deviceClient->get<bool>("timeTester", "slot_connected")) break;
        CPPUNIT_ASSERT(counter++ < 500);
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    m_deviceClient->execute("timeTester", "start");

    // some time to test the timing
    const unsigned int testDurationInMicrosec = 5432109u;
    boost::this_thread::sleep(boost::posix_time::microseconds(testDurationInMicrosec));

    m_deviceClient->execute("timeTester", "stop");

    const auto ids(m_deviceClient->get<std::vector<unsigned long long> >("timeTester", "ids"));
    const auto seconds(m_deviceClient->get<std::vector<unsigned long long> >("timeTester", "seconds"));
    const auto fractions(m_deviceClient->get<std::vector<unsigned long long> >("timeTester", "fractions"));

    // Test integrity, i.e. same size of vectors of ids and times
    CPPUNIT_ASSERT_EQUAL(ids.size(), seconds.size());
    CPPUNIT_ASSERT_EQUAL(ids.size(), fractions.size());
    CPPUNIT_ASSERT(ids.size() >= 2);

    // Test that ids are subsequent and time stamps are increasing (== is allowed!)
    unsigned long long lastId = ids[0];
    karabo::util::Epochstamp lastStamp(seconds[0], fractions[0]);
    for (size_t i = 1; i < ids.size(); ++i) {
        CPPUNIT_ASSERT_EQUAL(ids[i], lastId + 1ull);

        const karabo::util::Epochstamp currentStamp(seconds[i], fractions[i]);
        CPPUNIT_ASSERT(currentStamp >= lastStamp);

        const karabo::util::TimeDuration diff = (currentStamp - lastStamp);
        KARABO_LOG_FRAMEWORK_DEBUG_C("Timing_Test") << "diff for id: " << ids[i] << " " << static_cast<double> (diff);

        lastId = ids[i];
        lastStamp = currentStamp;
    }

    // Now test that the real ticks received from the time server have the expected spacing and are increasing
    // (== not allowed!).
    const auto idsTick(m_deviceClient->get<std::vector<unsigned long long> >("timeTester", "idsTick"));
    const auto secondsTick(m_deviceClient->get<std::vector<unsigned long long> >("timeTester", "secondsTick"));
    const auto fractionsTick(m_deviceClient->get<std::vector<unsigned long long> >("timeTester", "fractionsTick"));

    CPPUNIT_ASSERT(ids.size() > idsTick.size());
    CPPUNIT_ASSERT_EQUAL(idsTick.size(), secondsTick.size());
    CPPUNIT_ASSERT_EQUAL(idsTick.size(), fractionsTick.size());
    CPPUNIT_ASSERT(idsTick.size() >= 2);

    unsigned long long lastIdTick = idsTick[0];
    karabo::util::Epochstamp lastStampTick(secondsTick[0], fractionsTick[0]);
    for (size_t i = 1; i < idsTick.size(); ++i) {
        CPPUNIT_ASSERT_EQUAL(idsTick[i], lastIdTick + static_cast<unsigned long long> (tickCountdown));

        const karabo::util::Epochstamp currentStamp(secondsTick[i], fractionsTick[i]);
        CPPUNIT_ASSERT(currentStamp > lastStampTick);

        lastIdTick = idsTick[i];
        lastStampTick = currentStamp;
    }

    // As last test check how many ticks we really got - might be off a bit since time server sometimes reports
    // period that is off by periodVarFrac.
    const int numExpectedTicks = testDurationInMicrosec / tickPeriodInMicrosec;
    const int maxOff = static_cast<float> (std::ceil(tickCountdown * periodVarFrac));
    const std::string msg("Ids received: " + karabo::util::toString(ids.size())
                          + ", expected: " + karabo::util::toString(numExpectedTicks)
                          + ", maxOff: " + karabo::util::toString(maxOff));
    CPPUNIT_ASSERT_MESSAGE(msg, static_cast<int> (ids.size()) <= numExpectedTicks + maxOff);
    CPPUNIT_ASSERT_MESSAGE(msg, static_cast<int> (ids.size()) >= numExpectedTicks - maxOff);
}
