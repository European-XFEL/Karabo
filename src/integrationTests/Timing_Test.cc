/*
 * File:   Timing_Test.cc
 * Author: steffen.hauf@xfel.eu
 
 */

#include "Timing_Test.hh"
#include <karabo/net/EventLoop.hh>
#include <cstdlib>


using namespace std;

#define KRB_TEST_MAX_TIMEOUT 5
#define TICK_RATE 100 //ms

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
    
    // bring up a GUI server and a tcp adapter to it
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerTiming", "SimulatedTimeServerDevice",
                                                                       Hash("deviceId", "Karabo_TimeServer", "period", TICK_RATE, "updateRate", TICK_RATE), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    success = m_deviceClient->instantiate("testServerTiming", "TimingTestDevice", Hash("deviceId", "timeTester", "useTimeserver", true), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    m_lastCheck = karabo::util::Epochstamp();
    
    testSynchronized();
    testIntermittentUpdates();
    testTickStability();

}

void Timing_Test::testSynchronized() {
    // Here we test synchronized ticking - the time server determines the signal
    boost::this_thread::sleep(boost::posix_time::milliseconds(TICK_RATE*50));
    unsigned long long period = m_deviceClient->get<unsigned long long>("timeTester", "period");
    unsigned long long update_period = m_deviceClient->get<unsigned long long>("timeTester", "update_period");
    unsigned long long tick_count = m_deviceClient->get<unsigned long long>("timeTester", "tick_count");
    
    karabo::util::TimeDuration dt = (karabo::util::Epochstamp()-m_lastCheck);
    float expected_ticks = (dt.getSeconds()*TICK_RATE*10 + dt.getFractions()/1e6)/TICK_RATE;
    CPPUNIT_ASSERT(abs(period - update_period)/period <= 0.01);
    CPPUNIT_ASSERT(abs(period - TICK_RATE)/period <= 0.01);
    CPPUNIT_ASSERT(abs(update_period - TICK_RATE)/period <= 0.01);

    CPPUNIT_ASSERT(abs((tick_count - expected_ticks)/expected_ticks) <= 0.01); // allow for 1% error

}

void Timing_Test::testIntermittentUpdates(){
    // Here we test de-synchronized ticking - the time server only give intermittent sync. updates
    m_deviceClient->set("Karabo_TimeServer", "updateRate", TICK_RATE*10);
    boost::this_thread::sleep(boost::posix_time::milliseconds(TICK_RATE*50));
    unsigned long long server_rate = m_deviceClient->get<unsigned long long>("Karabo_TimeServer", "updateRate");
    CPPUNIT_ASSERT(server_rate == TICK_RATE *10);
    unsigned long long period = m_deviceClient->get<unsigned long long>("timeTester", "period");
    unsigned long long update_period = m_deviceClient->get<unsigned long long>("timeTester", "update_period");
    CPPUNIT_ASSERT(abs(period - update_period)/period <= 0.01);
    CPPUNIT_ASSERT(abs(period - TICK_RATE)/period <= 0.01);
    CPPUNIT_ASSERT(abs(update_period - TICK_RATE)/period <= 0.01); 
}

void Timing_Test::testTickStability() {
    // This tests tick stability, i.e. the updates a device receives. The tick interval/period shouldn't exceed the server tick rate
    // both in synced and de-synced server operation. Shorter updates are possible, e.g. when a sigal from the server takes precedence
    std::vector<unsigned long long> update_periods = m_deviceClient->get<std::vector<unsigned long long> >("timeTester", "update_periods");
    for(auto it = ++(update_periods.cbegin()); it != update_periods.cend(); ++it) { // first one will be off, because tester device needs to init
        CPPUNIT_ASSERT(*it <= TICK_RATE*1.01); // allow for 1% jitter
    }
}
