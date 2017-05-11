/* 
 * File:   TimingTestDevice.cc
 * Author: steffen.hauf@xfel.eu
 * 
 */

#include "TimingTestDevice.hh"

using namespace std;

USING_KARABO_NAMESPACES

        namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, TimingTestDevice)

    void TimingTestDevice::expectedParameters(Schema& expected) {
        UINT64_ELEMENT(expected).key("period")
            .unit(karabo::util::Unit::SECOND)
            .metricPrefix(karabo::util::MetricPrefix::MILLI)
            .readOnly().initialValue(100)
            .commit();
        
        UINT64_ELEMENT(expected).key("update_period")
            .unit(karabo::util::Unit::SECOND)
            .metricPrefix(karabo::util::MetricPrefix::MILLI)
            .readOnly().initialValue(100)
            .commit();
        
        VECTOR_UINT64_ELEMENT(expected).key("update_periods")
            .unit(karabo::util::Unit::SECOND)
            .metricPrefix(karabo::util::MetricPrefix::MILLI)
            .readOnly().initialValue(std::vector<unsigned long long>())
            .commit();
        
        
        UINT64_ELEMENT(expected).key("tick_count")
            .readOnly().initialValue(0)
            .commit();
        
        BOOL_ELEMENT(expected).key("slot_connected")
            .readOnly().initialValue(false)
            .commit();
      
    }


    TimingTestDevice::TimingTestDevice(const karabo::util::Hash& config) : Device<>(config) {
        KARABO_INITIAL_FUNCTION(initialize);
    }


    TimingTestDevice::~TimingTestDevice() {
    }


    void TimingTestDevice::initialize() {
        boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
        updateState(karabo::util::State::NORMAL);
    }
    
    void TimingTestDevice::onTimeUpdate(unsigned long long id, unsigned long long sec, unsigned long long frac, unsigned long long period) {
        karabo::util::Epochstamp now = karabo::util::Epochstamp();
        set("period", period);
        karabo::util::TimeDuration dt = (now - m_lastTimeStamp);
        unsigned long long update_period = dt.getSeconds()*1000 + dt.getFractions()/1000000u;
        set("update_period", update_period);
        m_lastTimeStamp = now;

        set("tick_count", get<unsigned long long>("tick_count") + 1);
        set("slot_connected", true);
        
        std::vector<unsigned long long> update_periods = get<std::vector<unsigned long long> >("update_periods");
        update_periods.push_back(update_period);
        set("update_periods", update_periods);
    }

}