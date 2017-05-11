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
            .metricPrefix(karabo::util::MetricPrefix::MILLI)
            .assignmentOptional().defaultValue(100)
            .reconfigurable()
            .commit();
        
        UINT64_ELEMENT(expected).key("updateRate")
            .unit(karabo::util::Unit::SECOND)
            .metricPrefix(karabo::util::MetricPrefix::MILLI)
            .assignmentOptional().defaultValue(5000)
            .reconfigurable()
            .commit();
      
    }


    SimulatedTimeServerDevice::SimulatedTimeServerDevice(const karabo::util::Hash& config) : Device<>(config), m_id(0), m_timeTickerTimer(karabo::net::EventLoop::getIOService()) {
        KARABO_INITIAL_FUNCTION(initialize);
        KARABO_SYSTEM_SIGNAL4("signalTimeTick", unsigned long long /*id */, unsigned long long /* sec */, unsigned long long /* frac */, long long /* period */);
    }


    SimulatedTimeServerDevice::~SimulatedTimeServerDevice() {
        m_timeTickerTimer.cancel();
    }


    void SimulatedTimeServerDevice::initialize() {
        unsigned long long period = get<unsigned long long>("period");
        m_lastTimeStamp = karabo::util::Epochstamp();
        m_timeTickerTimer.expires_from_now(boost::posix_time::milliseconds(period));
        m_timeTickerTimer.async_wait(util::bind_weak(&SimulatedTimeServerDevice::tickTock, this, boost::asio::placeholders::error));   
    }
    
    void SimulatedTimeServerDevice::tickTock(const boost::system::error_code& e) {

        long long period = get<unsigned long long>("period");
        unsigned long long update_rate = get<unsigned long long>("updateRate");
        karabo::util::Epochstamp now = karabo::util::Epochstamp();
        karabo::util::TimeDuration dt = (now - m_lastTimeStamp);
        if (((dt.getSeconds()*1000 + dt.getFractions()/1000000u) >= update_rate) || (m_last_update_rate != update_rate) ){
            emit("signalTimeTick", m_id, now.getSeconds(), now.getFractionalSeconds(), (update_rate <= period) ? period : -period);
            m_lastTimeStamp = now;
        }
        m_id++;
        m_last_update_rate = update_rate;
        m_timeTickerTimer.expires_from_now(boost::posix_time::milliseconds(period));
        m_timeTickerTimer.async_wait(util::bind_weak(&SimulatedTimeServerDevice::tickTock, this, boost::asio::placeholders::error));
        
    }

}