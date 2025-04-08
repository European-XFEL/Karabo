/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   SimulatedTimeServerDevice.cc
 * Author: steffen.hauf@xfel.eu
 *
 */

#include "SimulatedTimeServerDevice.hh"

#include <chrono>

using namespace std;
using namespace std::chrono;

USING_KARABO_NAMESPACES

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(Device, SimulatedTimeServerDevice)

    void SimulatedTimeServerDevice::expectedParameters(Schema& expected) {
        UINT64_ELEMENT(expected)
              .key("initialId")
              .displayedName("Initial Id")
              .description("First id published")
              .assignmentOptional()
              .defaultValue(1ull)
              .commit();

        UINT64_ELEMENT(expected)
              .key("period")
              .unit(karabo::data::Unit::SECOND)
              .metricPrefix(karabo::data::MetricPrefix::MICRO)
              .assignmentOptional()
              .defaultValue(100000ull)
              .reconfigurable()
              .commit();

        UINT32_ELEMENT(expected)
              .key("tickCountdown")
              .displayedName("Tick countdown")
              .description("The number defining which tick should be broadcasted, i.e. 10 means 'every tenth tick'")
              .assignmentOptional()
              .defaultValue(10)
              .minInc(1)
              .reconfigurable()
              .commit();

        FLOAT_ELEMENT(expected)
              .key("periodVariationFraction")
              .displayedName("Period variation factor")
              .description(
                    "Whenever broadcasting the tick, give a period length that is correct, too short or too long by "
                    "this fraction")
              .assignmentOptional()
              .defaultValue(0.f) // no variation by default
              .minInc(0.f)
              .maxInc(.9f)
              .reconfigurable()
              .commit();

        SLOT_ELEMENT(expected)
              .key("resetId")
              .displayedName("Reset id")
              .description("Reset ids to start again with 1")
              .commit();
    }


    SimulatedTimeServerDevice::SimulatedTimeServerDevice(const karabo::data::Hash& config)
        : Device(config),
          m_id(config.get<unsigned long long>("initialId")),
          m_emitCount(0ull),
          m_timeTickerTimer(karabo::net::EventLoop::getIOService()) {
        KARABO_INITIAL_FUNCTION(initialize);
        KARABO_SIGNAL("signalTimeTick", unsigned long long /*id */, unsigned long long /* sec */,
                      unsigned long long /* frac */, unsigned long long /* period */);
        KARABO_SLOT(resetId);
    }


    SimulatedTimeServerDevice::~SimulatedTimeServerDevice() {
        m_timeTickerTimer.cancel();
    }


    void SimulatedTimeServerDevice::initialize() {
        m_tickCountdown = 0;
        const unsigned long long period = get<unsigned long long>("period");

        m_timeTickerTimer.expires_after(microseconds(period));
        m_timeTickerTimer.async_wait(
              util::bind_weak(&SimulatedTimeServerDevice::tickTock, this, boost::asio::placeholders::error));
    }


    void SimulatedTimeServerDevice::tickTock(const boost::system::error_code& e) {
        if (e) return;

        const unsigned long long period = get<unsigned long long>("period");
        const karabo::data::Epochstamp now;

        if (m_tickCountdown == 0) {
            KARABO_LOG_FRAMEWORK_DEBUG << "ticktock emits: " << m_id << " " << m_tickCountdown << " at "
                                       << now.getSeconds() << " " << now.getFractionalSeconds();
            m_tickCountdown = get<unsigned int>("tickCountdown");   // re-arm the counter
            const unsigned long long sign = (m_emitCount++ % 3ull); // 0, 1, or 2
            unsigned long long fakePeriod = period;
            if (sign) {
                const auto periodDiff = static_cast<unsigned long long>(get<float>("periodVariationFraction") * period);
                if (sign == 1) {
                    fakePeriod -= periodDiff;
                } else { // i.e. 2
                    fakePeriod += periodDiff;
                }
            }
            emit("signalTimeTick", static_cast<unsigned long long>(m_id), now.getSeconds(), now.getFractionalSeconds(),
                 fakePeriod);
        } else {
            KARABO_LOG_FRAMEWORK_DEBUG << "ticktock does NOT emit: " << m_id << " " << m_tickCountdown << " at "
                                       << now.getSeconds() << " " << now.getFractionalSeconds();
        }
        --m_tickCountdown;

        //
        m_timeTickerTimer.expires_at(m_timeTickerTimer.expires_at() + microseconds(period));
        m_timeTickerTimer.async_wait(
              util::bind_weak(&SimulatedTimeServerDevice::tickTock, this, boost::asio::placeholders::error));
        ++m_id;
    }


    void SimulatedTimeServerDevice::resetId() {
        m_id = 1ull;
    }

} // namespace karabo
