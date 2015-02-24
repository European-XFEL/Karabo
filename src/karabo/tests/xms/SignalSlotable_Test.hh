/*
 * File:   SignalSlotable_Test.hh
 * Author: heisenb
 *
 * Created on Apr 4, 2013, 1:24:21 PM
 */

#ifndef SIGNALSLOTABLE_TEST_HH
#define	SIGNALSLOTABLE_TEST_HH

#include <karabo/xms.hpp>
#include <cppunit/extensions/HelperMacros.h>

class SignalSlotDemo : public karabo::xms::SignalSlotable {
    
    int m_messageCount;
    bool m_allOk;
    boost::mutex m_mutex;

public:

    KARABO_CLASSINFO(SignalSlotDemo, "SignalSlotDemo", "1.0")

    SignalSlotDemo(const std::string& instanceId, const karabo::net::BrokerConnection::Pointer connection) : 
    karabo::xms::SignalSlotable(instanceId, connection), m_messageCount(0), m_allOk(true) {

        KARABO_SIGNAL("signalA", std::string);

        KARABO_SLOT(slotA, std::string);

        KARABO_SLOT(slotB, int, karabo::util::Hash);
        
        KARABO_SLOT(slotC, int);

    }

    void slotA(const std::string& msg) {
        //std::cout << "\nslotA : msg=" << msg << ", m_messageCount=" << m_messageCount << std::endl;
        boost::mutex::scoped_lock lock(m_mutex);
        // Assertions
        m_messageCount++;
        if (msg != "Hello World!") m_allOk = false;
        if (getSenderInfo("slotA")->getInstanceIdOfSender() != "SignalSlotDemo") {
            m_messageCount += 1000; // Invalidate message count will let the test fail!
        }
        SIGNAL2("signalB", int, karabo::util::Hash);
        connectN("signalB", "slotB");
        emit("signalB", 42, karabo::util::Hash("Was.soll.das.bedeuten", "nix"));
        //std::cout << "\nslotA END" << ", m_messageCount=" << m_messageCount << std::endl;
    }

    void slotB(int someInteger, const karabo::util::Hash& someConfig) {
        //std::cout << "\nslotB : someInteger = " << someInteger << ", and someConfig ...\n" << someConfig << ", m_messageCount=" << m_messageCount << std::endl;
        boost::mutex::scoped_lock lock(m_mutex);
        // Assertions
        m_messageCount++;
        if (someInteger != 42) m_allOk = false;
        if (someConfig.get<std::string>("Was.soll.das.bedeuten") != "nix") m_allOk = false;
        //std::cout << "\nslotB END" << ", m_messageCount=" << m_messageCount << std::endl;
    }
    
    void slotC(int number) {
        //std::cout << "\nslotC : number = " << number << ", m_messageCount=" << m_messageCount << std::endl;
         boost::mutex::scoped_lock lock(m_mutex);
        // Assertions
        m_messageCount++;
        if (number != 1) m_allOk = false;
        reply(number + number);
        //std::cout << "\nslotC END" << ", m_messageCount=" << m_messageCount << std::endl;
    }
    
    bool wasOk() {
        //std::cout << "\nwasOk : m_messageCount=" << m_messageCount << ", m_allOk=" << m_allOk << std::endl;
        return ((m_messageCount == 5) && m_allOk);
    }
    
    void myCallBack(const std::string& someData, int number) {
        std::clog << "Got called with: " << someData << " and " << number << std::endl;
    }


};

class SignalSlotable_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(SignalSlotable_Test);

    CPPUNIT_TEST(testMethod);
   
    CPPUNIT_TEST_SUITE_END();

public:
    SignalSlotable_Test();
    virtual ~SignalSlotable_Test();
    void setUp();
    void tearDown();

private:
    
    void testMethod();
};

#endif	/* SIGNALSLOTABLE_TEST_HH */

