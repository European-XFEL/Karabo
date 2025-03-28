/*
 * $Id$
 *
 * Author: burkhard.heisen@xfel.eu>
 *
 * Created on February 9, 2011, 2:24 PM
 *
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

#ifndef KARABO_CORE_DEVICESERVER_HH
#define KARABO_CORE_DEVICESERVER_HH

#include <spdlog/spdlog.h>

#include <boost/asio/deadline_timer.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Device.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/Strand.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/PluginLoader.hh"
#include "karabo/util/State.hh"
#include "karabo/util/Version.hh"
#include "karabo/xms/SignalSlotable.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        /**
         * @class DeviceServer
         * @brief The DeviceServer class hosts device instances
         *
         * The DeviceServer class hosts device instances. It monitors the system
         * for new class plugins appearing and notifies the distributed system
         * of these and their static information.
         *
         * The device server uses an ok-error FSM, which knows the ERROR and
         * NORMAL karabo::util::State states.
         */
        class DeviceServer : public karabo::xms::SignalSlotable {
            bool m_serverIsRunning;
            std::vector<karabo::util::Hash> m_autoStart;
            std::vector<std::string> m_deviceClasses;

            typedef std::unordered_map<std::string, std::pair<BaseDevice::Pointer, karabo::net::Strand::Pointer> >
                  DeviceInstanceMap;
            DeviceInstanceMap m_deviceInstanceMap;
            std::mutex m_deviceInstanceMutex;
            std::map<std::string, unsigned int> m_deviceInstanceCount;
            int m_visibility;

            karabo::net::Broker::Pointer m_connection;

            std::string m_serverId;
            std::string m_timeServerId;
            unsigned long long m_timeId;
            unsigned long long m_timeSec;    // seconds
            unsigned long long m_timeFrac;   // attoseconds
            unsigned long long m_timePeriod; // microseconds
            bool m_noTimeTickYet;            // whether slotTimeTick received a first call
            mutable std::mutex m_timeChangeMutex;
            unsigned long long m_timeIdLastTick; // only for onTimeTick, no need for mutex protection
            boost::asio::system_timer m_timeTickerTimer;
            std::string m_hostname;

           public:
            KARABO_CLASSINFO(DeviceServer, "DeviceServer", karabo::util::Version::getVersion())
            KARABO_CONFIGURATION_BASE_CLASS


            /**
             * Static properties of the device server
             * @param to inject these properties to
             */
            static void expectedParameters(karabo::util::Schema&);

            /**
             * The constructor expects a configuration Hash. The following
             * configuration options are supported:
             *
             * - serverId: a string giving the server's id
             * - autostart: a vector of Hashes containing configurations for device that are to be automatically started
             * by the server
             * - scanPlugins: a boolean indicating if this server should scan for additional plugins
             * - visibility: an integer indicating device server visibility in the distributed system
             * - debugMode: a boolean indicating if the server should run in debugMode
             * - connection: a Hash containing the connection information for a karabo::net::Broker
             * - pluginDirectory: a path to the plugin directory for this device server
             * - heartbeatInterval: interval in seconds at which this server sends heartbeats to the distributed system
             * - nThreads: number of threads to use in this device server
             * @param
             */
            DeviceServer(const karabo::util::Hash&);

            virtual ~DeviceServer();

            void finalizeInternalInitialization();

            /**
             * Check if the device server is running
             * @return
             */
            bool isRunning() const;

            void autostartDevices();

            /**
             * This function was called before in case of error events in FSM. Now it is a candidate
             * to be removed since we don't have any references to it in DeviceServer code.
             * @param user level error message
             * @param detail level error message
             */
            void errorFoundAction(const std::string& user, const std::string& detail);

            /**
             * It just launches devices that marked to be started by server automatically.
             */
            void startInitialActions() {
                autostartDevices();
            }

           private: // Functions
            karabo::util::Hash availablePlugins();

            void slotStartDevice(const karabo::util::Hash& configuration);

            void startDevice(const karabo::util::Hash& configuration, const SignalSlotable::AsyncReply& reply);

            void loadLogger(const karabo::util::Hash& input);

            void slotKillServer();

            void slotLoggerContent(const karabo::util::Hash& input);

            void stopDeviceServer();

            std::shared_ptr<spdlog::logger> log();

            void registerSlots();

            void slotDeviceGone(const std::string& instanceId);

            void slotGetClassSchema(const std::string& classId);

            std::string generateDefaultServerId() const;

            std::string generateDefaultDeviceId(const std::string& classId);

            /// Helper to create input passed to instantiate.
            /// Returns a tuple of the deviceId, the classId and the configuration.
            std::tuple<std::string, std::string, util::Hash> prepareInstantiate(const util::Hash& configuration);

            /// Helper for instantiateDevices - e.g. provides the (async) reply for slotStartDevice.
            void instantiate(const std::string& deviceId, const std::string& classId, const util::Hash& config,
                             const SignalSlotable::AsyncReply& asyncReply);

            void slotLoggerPriority(const std::string& prio);

            /**
             * A slot called by the time-server to synchronize this device with the timing system.
             *
             * @param id: current train id
             * @param sec: current system seconds
             * @param frac: current fractional seconds
             * @param period: interval between subsequent ids in microseconds
             */
            void slotTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac,
                              unsigned long long period);

            /**
             * Helper function for internal time ticker deadline timer to provide internal clock
             * that calls 'onTimeUpdate' for every id even if slotTimeTick is called less often.
             *
             * @param ec error code indicating whether deadline timer was cancelled
             * @param id: current train id
             */
            void timeTick(const boost::system::error_code ec, unsigned long long newId);

            void onBroadcastMessage(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);
        };

    } // namespace core
} // namespace karabo


#endif
