/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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
        SLOT_ELEMENT(expected).key("start").commit();

        SLOT_ELEMENT(expected).key("stop").commit();

        VECTOR_UINT64_ELEMENT(expected).key("ids").readOnly().initialValue(std::vector<unsigned long long>()).commit();

        VECTOR_UINT64_ELEMENT(expected)
              .key("seconds")
              .description("Full seconds of the received time updates")
              .readOnly()
              .initialValue(std::vector<unsigned long long>())
              .commit();

        VECTOR_UINT64_ELEMENT(expected)
              .key("fractions")
              .description("Fractions of seconds of the received time updates")
              .readOnly()
              .initialValue(std::vector<unsigned long long>())
              .commit();

        VECTOR_UINT64_ELEMENT(expected)
              .key("idsTick")
              .readOnly()
              .initialValue(std::vector<unsigned long long>())
              .commit();

        VECTOR_UINT64_ELEMENT(expected)
              .key("secondsTick")
              .description("Full seconds of the calls to slotTimeTick")
              .readOnly()
              .initialValue(std::vector<unsigned long long>())
              .commit();

        VECTOR_UINT64_ELEMENT(expected)
              .key("fractionsTick")
              .description("Fractions of seconds of the calls to slotTimeTick")
              .readOnly()
              .initialValue(std::vector<unsigned long long>())
              .commit();

        BOOL_ELEMENT(expected).key("slot_connected").readOnly().initialValue(false).commit();
    }


    TimingTestDevice::TimingTestDevice(const karabo::util::Hash& config) : Device<>(config), m_started(false) {
        KARABO_SLOT(start);
        KARABO_SLOT(stop);
    }


    TimingTestDevice::~TimingTestDevice() {}


    void TimingTestDevice::start() {
        m_started = true;
    }


    void TimingTestDevice::stop() {
        m_started = false;

        // Some sleep to guarantee that any onTimeUpdate has finished to avoid races on members.
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));

        set(Hash("ids", m_ids, "seconds", m_seconds, "fractions", m_fractions, "idsTick", m_idsTick, "secondsTick",
                 m_secondsTick, "fractionsTick", m_fractionsTick));

        m_ids.clear();
        m_seconds.clear();
        m_fractions.clear();

        m_idsTick.clear();
        m_secondsTick.clear();
        m_fractionsTick.clear();
    }


    void TimingTestDevice::onTimeUpdate(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                        unsigned long long period) {
        if (!get<bool>("slot_connected")) {
            set("slot_connected", true);
        }
        if (!m_started) return;

        m_ids.push_back(id);
        m_seconds.push_back(sec);
        m_fractions.push_back(frac);
    }


    void TimingTestDevice::onTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                      unsigned long long period) {
        if (!m_started) return;

        m_idsTick.push_back(id);
        m_secondsTick.push_back(sec);
        m_fractionsTick.push_back(frac);
    }
} // namespace karabo
