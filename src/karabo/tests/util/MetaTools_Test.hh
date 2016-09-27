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

struct SignalSlotable : public boost::enable_shared_from_this<SignalSlotable> {

    virtual ~SignalSlotable() {
    }

};

struct Device : public virtual SignalSlotable {

    boost::asio::deadline_timer m_timer;
    std::vector<std::string>* m_messages;
    boost::mutex m_messageMutex;

    Device(std::vector<std::string>* messages)
        : m_timer(karabo::net::EventLoop::getIOService()), m_messages(messages) {
        {
            boost::mutex::scoped_lock lock(m_messageMutex);
            m_messages->push_back("Device created");
        }
    }

    void init() {
        m_timer.expires_from_now(boost::posix_time::millisec(100));
        m_timer.async_wait(karabo::util::bind_weak(&Device::executeStepFunction, this, 5, _1));
    }

    ~Device() {
        {
            boost::mutex::scoped_lock lock(m_messageMutex);
            m_messages->push_back("Device deleted");
        }
        m_timer.cancel();
    }

    void executeStepFunction(int arg, const boost::system::error_code& error) {
        // Canceled timer
        if (error == boost::asio::error::operation_aborted) {
            {
                boost::mutex::scoped_lock lock(m_messageMutex);
                m_messages->push_back("Timer aborted");
            }
            return;
        }
        // Schedule next step
        {
            boost::mutex::scoped_lock lock(m_messageMutex);
            m_messages->push_back("Tick " + boost::lexical_cast<std::string>(arg));
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(50));

        m_timer.expires_from_now(boost::posix_time::millisec(500));

        m_timer.async_wait(karabo::util::bind_weak(&Device::executeStepFunction, this, arg + 1, _1));
    }
};

struct DeviceServer {

    boost::asio::deadline_timer m_deviceDestructTimer;
    std::map<std::string, boost::shared_ptr<Device> > m_devices;

    DeviceServer(std::vector<std::string>* messages)
        : m_deviceDestructTimer(karabo::net::EventLoop::getIOService()) {
        m_devices["someDevice"] = boost::shared_ptr<Device>(new Device(messages));
        m_devices["someDevice"]->init();

        m_deviceDestructTimer.expires_from_now(boost::posix_time::millisec(1500));
        m_deviceDestructTimer.async_wait(boost::bind(&DeviceServer::killDevice, this, _1, "someDevice"));
    }

    void killDevice(const boost::system::error_code& error, const std::string& deviceName) {
        m_devices.erase(deviceName);
    }
};

class MetaTools_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(MetaTools_Test);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testWeakBind);
    CPPUNIT_TEST_SUITE_END();

public:
    MetaTools_Test();
    virtual ~MetaTools_Test();

private:

    void testMethod();
    void testWeakBind();

};

#endif	/* METATOOLS_TEST_HH */

