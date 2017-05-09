/*
 * File:   Timing_Test.cc
 * Author: steffen.hauf@xfel.eu
 
 */

#include "Timing_Test.hh"
#include <karabo/net/EventLoop.hh>
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
   setenv("KARABO_BROKER", "tcp://localhost:7777", true);
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
                                                                       Hash("deviceId", "Karabo_TimeServer", "period", 100, "updateRate", 100), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    success = m_deviceClient->instantiate("testServerTiming", "TimingTestDevice", Hash("deviceId", "timeTester", "useTimeserver", true), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    
    int tries = 20;
    
    /*while(!m_deviceClient->get<bool>("timeTester", "slot_connected")) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        tries--;
        std::clog<<tries<<std::endl;
        if (tries == 0) {
            std::clog<<"Failed connecting to slotTimeTick"<<std::endl;
            CPPUNIT_ASSERT(false);
        }
    }*/
    m_lastCheck = karabo::util::Epochstamp();
    
    testCorrelated();
    testIntermittantUpdates();
   

}

void Timing_Test::testCorrelated() {
    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
    unsigned long long period = m_deviceClient->get<unsigned long long>("timeTester", "period");
    unsigned long long update_period = m_deviceClient->get<unsigned long long>("timeTester", "update_period");
    unsigned long long tick_count = m_deviceClient->get<unsigned long long>("timeTester", "tick_count");
    
    float expected_ticks = (karabo::util::Epochstamp()-m_lastCheck).getFractions()/1e9.;
    
    CPPUNIT_ASSERT(abs(period - update_period)/period <= 0.01);
    CPPUNIT_ASSERT(abs(period - 100)/period <= 0.01);
    CPPUNIT_ASSERT(abs(update_period - 100)/period <= 0.01);
    std::clog<<tick_count<<" "<<expected_ticks<<" "<<abs((float)(tick_count - expected_ticks)/expected_ticks)<<std::endl;
    CPPUNIT_ASSERT(abs((tick_count - expected_ticks)/expected_ticks) <= 0.25); // allow for 25% error

}

void Timing_Test::testIntermittantUpdates(){
    m_deviceClient->set("Karabo_TimeServer", "updateRate", 1000);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    unsigned long long server_rate = m_deviceClient->get<unsigned long long>("Karabo_TimeServer", "updateRate");
   /* CPPUNIT_ASSERT(server_rate == 1000);
    unsigned long long period = m_deviceClient->get<unsigned long long>("timeTester", "period");
    unsigned long long update_period = m_deviceClient->get<unsigned long long>("timeTester", "update_period");
    CPPUNIT_ASSERT(abs(period - update_period)/period <= 0.01);
    CPPUNIT_ASSERT(abs(period - 100)/period <= 0.01);
    CPPUNIT_ASSERT(abs(update_period - 100)/period <= 0.01);*/
    
}
