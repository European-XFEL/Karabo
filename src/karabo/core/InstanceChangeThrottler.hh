/*
 * File:   InstanceMessageThrottler.hh
 * Author: <raul.costa@xfel.eu>
 *
 * Created on December 3, 2018, 10:52 AM
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

#ifndef KARABO_CORE_INSTANCEMESSAGETHROTTLER_HH
#define KARABO_CORE_INSTANCEMESSAGETHROTTLER_HH

#include <boost/asio/steady_timer.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>

#include "karabo/data/types/ClassInfo.hh"
#include "karabo/data/types/Hash.hh"

namespace karabo {

    namespace core {

        /**
         * @brief Receives instance new, update and gone messages and dispatches them to an interested part
         * in "cycles" spaced by a given interval. Also takes care of removing redundant message sequences.
         *
         * @note
         * The hash is composed of three level of hashes. The root hash has the types of the instances changes as
         * its keys . The possible values for this first level keys are "new", "gone" and "update".
         * The three keys will always be present in the root hash, even when a particular cycle has no change of the
         * given type to dispatch.
         *
         * The second level hashes are the values of the root hash. Their keys are the types of the instances whose
         * changes are on the third level hashes. The keys are the contents of the InstanceInfo.Type field in the
         * instance change data. Typical values for second level keys would be "device", "server" and "macro".
         *
         * The third level hashes are the values of the second level hashes. Their keys will be the instanceIds in
         * the instance change data. Those keys can be either a deviceId, a serverId or any other kind of Id,
         * depending on the type of the instance. The the third level hashes will be the ones resulting
         * from calling the InstanceChangeEntryEncoder function passed as an argument to the Throttler with the
         * InstanceId and InstanceInfo in the instance change data. For "new" and "update" changes the third level
         * hash will be an empty hash with the input InstanceInfo fields as attributes. For "gone" changes the third
         * level hash will not be empty and will have the same lay-out as the input InstanceInfo hash.
         */
        class InstanceChangeThrottler : public std::enable_shared_from_this<InstanceChangeThrottler> {
           public:
            KARABO_CLASSINFO(InstanceChangeThrottler, "InstanceMessageThrottler", "2.0")

            typedef std::function<void(const karabo::data::Hash&)> InstanceChangeHandler;

            enum class InstChangeType { NEW, UPDATE, GONE };

            /**
             * InstanceChangeThrottler factory.
             *
             * @param instChangeHandler The handler for instance change events dispatched by the Throttler.
             * @param cycleIntervalMs The interval in milliseconds between throttler cycles.
             * @param maxChangesPerCycle The maximum number of instance changes entries to be dispatched per
             * throttler cycle. If this limit is reached before the throttler interval elapses, a cycle is started
             * immediately to dispatch the changes to the handler.
             *
             * @return A shared pointer to an InstanceChangeThrottler.
             *
             * @note The Throttler only has a private constructor; every instantiation of a Throttler must come from
             * this factory method. It takes care of initializing the newly instantiated Throttler.
             *
             * @note instChangeEntryEncoder has been added to allow the Throttler to call
             * DeviceClient::prepareTopologyEntry without directly knowing DeviceClient.
             *
             */
            static std::shared_ptr<InstanceChangeThrottler> createThrottler(
                  const InstanceChangeHandler& instChangeHandler, unsigned int cycleIntervalMs = 500u,
                  unsigned int maxChangesPerCycle = 100);

            virtual ~InstanceChangeThrottler();

            /**
             * Submits an instance new change for dispatching by the throttler.
             *
             * @param instanceId The id of the instance the new change refers to.
             * @param instanceInfo Information about the instance the new change refers to.
             */
            void submitInstanceNew(const std::string& instanceId, const karabo::data::Hash& instanceInfo);

            /**
             * Submits an instance update change for dispatching by the throttler.
             *
             * @param instanceId The id of the instance the update change refers to.
             * @param instanceInfo Information about the instance the update change refers to.
             */
            void submitInstanceUpdate(const std::string& instanceId, const karabo::data::Hash& instanceInfo);

            /**
             * Submits an instance gone change for dispatching by the throttler.
             *
             * @param instanceId The id of the instance the gone change refers to.
             * @param instanceInfo Information about the instance the gone change refers to.
             */
            void submitInstanceGone(const std::string& instanceId, const karabo::data::Hash& instanceInfo);

            /**
             * The interval, in milliseconds, between cycles of the throttler.
             */
            unsigned int cycleIntervalMs() const;

            /**
             * The maximum number of instance changes entries to be dispatched per throttler cycle. If this limit is
             * reached before the throttler interval elapses, a cycle is started immediately to dispatch the changes to
             * the registered handler.
             */
            unsigned int maxChangesPerCycle() const;


            /**
             * Returns the string representation of a given InstChangeType value.
             */
            std::string getInstChangeTypeStr(InstChangeType changeType) const;

            /**
             * Flushes the throttler by making it dispatch the instance changes it has stored asap.
             *
             * @note this is a wrapper for the private flushThrottler(bool) method.
             */
            void flush();

           private:
            explicit InstanceChangeThrottler(const InstanceChangeHandler& instChangeHandler,
                                             unsigned int cycleIntervalMs, unsigned int maxChangesPerCycle);


            /**
             * A Hash with all the instances changes to be dispatched by the Throttler in its next cycle.
             *
             * Description for the Hash format can be found in the class documentation.
             */
            karabo::data::Hash m_instChanges;

            // Protects against simultaneous accesses to m_instChanges.
            std::mutex m_instChangesMutex;

            void initCycleInstChanges();
            void resetCycleInstChanges();

            unsigned int m_cycleIntervalMs;
            unsigned int m_maxChangesPerCycle;

            // The number of changes to be dispatched in the next Throttler cycle (<=  m_maxChangesPerCycle).
            unsigned int m_totalChangesInCycle;

            boost::asio::steady_timer m_throttlerTimer;

            InstanceChangeHandler m_instChangeHandler;

            /**
             * Encodes the instanceInfo hash into the format that the Throttler uses internally for changes
             * of type NEW and UPDATE.
             * @param instanceId the id of the instance the change is about.
             * @param instanceInfo the instanceInfo hash as used by the karabo GUI.
             * @return a hash whose only key is the instanceId, with the keys/values in instanceInfo as attributes and
             * an empty hash as the only value.
             */
            karabo::data::Hash instNewUpdateEncoder(const std::string& instanceId,
                                                    const karabo::data::Hash& instanceInfo) const;

            /**
             * Encodes the instanceInfo hash into the format that the Throttler uses internally for changes
             * of type GONE.
             * @param instanceId the id of the instance the change is about.
             * @param instanceInfo the instanceInfo hash as used by the karabo GUI.
             * @return a hash whose only key is the instanceId and whose only value is the instanceInfo hash.
             */
            karabo::data::Hash instGoneEncoder(const std::string& instanceId,
                                               const karabo::data::Hash& instanceInfo) const;

            /**
             * Adds an instance change to m_instChanges.
             *
             * As part of the addition, performs some optimizations to the set of events already in the hash. It can
             * happen that the new change actually "cancels" a set of changes that had been previously added.
             * An example: an 'instance gone' event can "cancel" all the 'instance new' and 'instance update' events
             * related to the same instance; in this scenario, the 'addition' of the gone event would actually consist
             * of the removal of the other events related to the same instance.
             *
             * @param changeType
             * @param instanceId
             * @param instanceInfo
             */
            void addChange(InstChangeType changeType, const std::string& instanceId,
                           const karabo::data::Hash& instanceInfo);

            /**
             * Throttler cycle execution. For each cycle, the throttler dispatches the instances changes hash.
             *
             * @param e Error code passed by the boost::asio::deadline_timer ticking mechanism.
             */
            void runThrottlerCycleAsync(const boost::system::error_code& e);

            /**
             * Schedules the next throttler event dispatching cycle.
             */
            void kickNextThrottlerCycleAsync();

            /**
             * Flushes the throttler by running its dispatching loop immediately.
             *
             * @param if true, the next throttler cycle is scheduled after the flush completes.
             *
             * @note Assumes that the mutex for acessing instanceChange data is acquired by a caller (either the
             * direct caller or another caller down the activation stack).
             */
            void flushThrottler(bool kickNextCycle = true);
        };

    } // namespace core

} // namespace karabo


#endif /* KARABO_CORE_INSTANCEMESSAGETHROTTLER_HH */
