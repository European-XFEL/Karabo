/*
 * File:   MetaTools_Test.hh
 * Author: heisenb
 *
 * Created on August 11, 2016, 11:18 AM
 */

#ifndef METATOOLS_TEST_HH
#define	METATOOLS_TEST_HH

#include <karabo/util/Hash.hh>
#include <cppunit/extensions/HelperMacros.h>
#include <karabo/util/MetaTools.hh>
#include <karabo/net/EventLoop.hh>



#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <unordered_map>
#include <iostream>

struct MyPublicHash : public karabo::util::Hash {

};

struct MyProtectedHash : protected karabo::util::Hash {

};

struct MyPrivateHash : private karabo::util::Hash {

};

struct PointerTest {

    template<class T>
    static bool isSharedPointer() {
        return isSharedPointer<T>(karabo::util::is_shared_ptr<T>());
    }

    template<class T>
    static bool isSharedPointer(boost::true_type) {
        return true;
    }

    template<class T>
    static bool isSharedPointer(boost::false_type) {
        return false;
    }
};

struct Test_SignalSlotable : public boost::enable_shared_from_this<Test_SignalSlotable> {

    virtual ~Test_SignalSlotable() {
    }

};

struct Test_Device : public virtual Test_SignalSlotable {

    boost::asio::deadline_timer m_timer;
    std::vector<std::string>* m_messages;

    Test_Device(std::vector<std::string>* messages)
        : m_timer(karabo::net::EventLoop::getIOService()), m_messages(messages) {
        m_messages->push_back("Test_Device created");
    }

    void init() {
        m_timer.expires_from_now(boost::posix_time::millisec(100));
        m_timer.async_wait(karabo::util::bind_weak(&Test_Device::executeStepFunction, this, 5, _1));
    }

    ~Test_Device() {
        m_messages->push_back("Test_Device deleted");
        m_timer.cancel();
    }

    void executeStepFunction(int arg, const boost::system::error_code& error) {
        // Canceled timer
        if (error == boost::asio::error::operation_aborted) {
            m_messages->push_back("Timer aborted");
            return;
        }
        // Schedule next step
        m_messages->push_back("Tick " + boost::lexical_cast<std::string>(arg));

        boost::this_thread::sleep(boost::posix_time::milliseconds(50));

        m_timer.expires_from_now(boost::posix_time::millisec(500));

        m_timer.async_wait(karabo::util::bind_weak(&Test_Device::executeStepFunction, this, arg + 1, _1));
    }
};

struct Test_DeviceServer {

    boost::asio::deadline_timer m_deviceDestructTimer;
    std::map<std::string, boost::shared_ptr<Test_Device> > m_devices;

    Test_DeviceServer(std::vector<std::string>* messages)
        : m_deviceDestructTimer(karabo::net::EventLoop::getIOService()) {
        m_devices["someTest_Device"] = boost::shared_ptr<Test_Device>(new Test_Device(messages));
        m_devices["someTest_Device"]->init();

        m_deviceDestructTimer.expires_from_now(boost::posix_time::millisec(1500));
        m_deviceDestructTimer.async_wait(boost::bind(&Test_DeviceServer::killTest_Device, this, _1, "someTest_Device"));
    }

    void killTest_Device(const boost::system::error_code& error, const std::string& deviceName) {
        m_devices.erase(deviceName);
    }
};

class MetaTools_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(MetaTools_Test);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testWeakBind);
    CPPUNIT_TEST(testCastResolvers);
    CPPUNIT_TEST(testCallFromTuple);
    CPPUNIT_TEST_SUITE_END();

public:
    MetaTools_Test();
    virtual ~MetaTools_Test();

private:

    void testMethod();
    void testWeakBind();
    void testCastResolvers();
    void testCallFromTuple();

};

#endif	/* METATOOLS_TEST_HH */

