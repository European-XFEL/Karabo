/* 
 * File:   SimulatedTimeServerDevice.cc
 * Author: steffen.hauf@xfel.eu
 * 
 */

#include "SimulatedTimeServerDevice.hh"

using namespace std;

USING_KARABO_NAMESPACES

        namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SimulatedTimeServerDevice)

    void SimulatedTimeServerDevice::expectedParameters(Schema& expected) {
        UINT64_ELEMENT(expected).key("period")
            .unit(karabo::util::Unit::SECOND)
            .metricPrefix(karabo::util::MetricPrefix::MICRO)
            .assignmentOptional().defaultValue(100000ull)
            .reconfigurable()
            .commit();

        UINT32_ELEMENT(expected).key("tickCountdown")
            .displayedName("Tick countdown")
            .description("The number defining which tick should be broadcasted, i.e. 10 means 'every tenth tick'")
            .assignmentOptional().defaultValue(10)
            .minInc(1)
            .reconfigurable()
            .commit();
    }


    SimulatedTimeServerDevice::SimulatedTimeServerDevice(const karabo::util::Hash& config) : Device<>(config),
        m_id(1ull),
        m_timeTickerTimer(karabo::net::EventLoop::getIOService()) {

        karabo::net::EventLoop::addThread();

        KARABO_INITIAL_FUNCTION(initialize);
        KARABO_SYSTEM_SIGNAL("signalTimeTick", unsigned long long /*id */, unsigned long long /* sec */, unsigned long long /* frac */, unsigned long long /* period */);
    }


    SimulatedTimeServerDevice::~SimulatedTimeServerDevice() {
        m_timeTickerTimer.cancel();
        karabo::net::EventLoop::removeThread();
    }


    void SimulatedTimeServerDevice::initialize() {
        m_tickCountdown = 0;
        const unsigned long long period = get<unsigned long long>("period");
        m_lastTimeStamp = karabo::util::Epochstamp();

        m_timeTickerTimer.expires_from_now(boost::posix_time::microseconds(period));
        m_timeTickerTimer.async_wait(util::bind_weak(&SimulatedTimeServerDevice::tickTock, this, boost::asio::placeholders::error));   
    }
    
    void SimulatedTimeServerDevice::tickTock(const boost::system::error_code& e) {
        if (e) return;

        const unsigned long long period = get<unsigned long long>("period");
        const karabo::util::Epochstamp now;

        if (m_tickCountdown == 0) {
            KARABO_LOG_FRAMEWORK_DEBUG << "ticktock emits: " << m_id << " " << m_tickCountdown;
            m_tickCountdown = get<unsigned int>("tickCountdown"); // re-arm the counter
            emit("signalTimeTick", m_id, now.getSeconds(), now.getFractionalSeconds(), period);
        } else {
            KARABO_LOG_FRAMEWORK_DEBUG << "ticktock does NOT emit: " << m_id << " " << m_tickCountdown;
        }
        --m_tickCountdown;

        m_timeTickerTimer.expires_at(m_lastTimeStamp.getPtime() + boost::posix_time::microseconds(period));
        m_timeTickerTimer.async_wait(util::bind_weak(&SimulatedTimeServerDevice::tickTock, this, boost::asio::placeholders::error));

        m_lastTimeStamp = now;
        ++m_id;
    }

}