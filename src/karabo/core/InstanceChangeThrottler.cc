/*
 * File:   InstanceChangeThrottler.cc
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

#include "InstanceChangeThrottler.hh"

#include <boost/asio/placeholders.hpp>
#include <chrono>
#include <karabo/log/Logger.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/util/MetaTools.hh>

using namespace std::chrono;
using namespace karabo::util;

namespace karabo {

    namespace core {

        //<editor-fold desc="Construction & Destruction">


        std::shared_ptr<InstanceChangeThrottler> InstanceChangeThrottler::createThrottler(
              const InstanceChangeHandler& instChangeHandler, unsigned int cycleIntervalMs,
              unsigned int maxChangesPerCycle) {
            std::shared_ptr<InstanceChangeThrottler> pt(
                  new InstanceChangeThrottler(instChangeHandler, cycleIntervalMs, maxChangesPerCycle));
            pt->initCycleInstChanges();
            pt->kickNextThrottlerCycleAsync();
            return pt;
        }


        InstanceChangeThrottler::InstanceChangeThrottler(const InstanceChangeHandler& instChangeHandler,
                                                         unsigned int cycleIntervalMs, unsigned int maxChangesPerCycle)
            : m_cycleIntervalMs(cycleIntervalMs),
              m_maxChangesPerCycle(maxChangesPerCycle),
              m_throttlerTimer(karabo::net::EventLoop::getIOService()),
              m_instChangeHandler(instChangeHandler){};


        InstanceChangeThrottler::~InstanceChangeThrottler() {
            // The mutex lock below is an overkill - the factory method for this class returns
            // a shared_ptr to an instance of it, decreasing the chances of existing another
            // thread that refers to the instance being destroyed.
            std::lock_guard<std::mutex> lock(m_instChangesMutex);
            flushThrottler(false);
        }

        //</editor-fold>


        //<editor-fold desc="Submission of instances changes">


        Hash InstanceChangeThrottler::instNewUpdateEncoder(const std::string& instanceId,
                                                           const Hash& instanceInfo) const {
            Hash h(instanceId, Hash());
            for (Hash::const_iterator it = instanceInfo.begin(); it != instanceInfo.end(); ++it) {
                h.setAttribute(instanceId, it->getKey(), it->getValueAsAny());
            }
            return h;
        }


        void InstanceChangeThrottler::submitInstanceNew(const std::string& instanceId, const Hash& instanceInfo) {
            std::lock_guard<std::mutex> lock(m_instChangesMutex);

            const std::string& instType = instanceInfo.get<std::string>("type");
            std::string updateTypeKey = getInstChangeTypeStr(InstChangeType::UPDATE) + "." + instType;
            bool hasUpdate = m_instChanges.has(updateTypeKey + "." + instanceId);

            if (hasUpdate) {
                // Optimization: an update followed by a new is removed.
                Hash& updateTypeHash = m_instChanges.get<Hash>(updateTypeKey);
                updateTypeHash.erase(instanceId);
                m_totalChangesInCycle--;
            }

            addChange(InstChangeType::NEW, instanceId, instanceInfo);
        }


        void InstanceChangeThrottler::submitInstanceUpdate(const std::string& instanceId, const Hash& instanceInfo) {
            const std::string& instType = instanceInfo.get<std::string>("type");

            std::lock_guard<std::mutex> lock(m_instChangesMutex);

            std::string newTypeKey = getInstChangeTypeStr(InstChangeType::NEW) + "." + instType;
            std::string updateTypeKey = getInstChangeTypeStr(InstChangeType::UPDATE) + "." + instType;

            bool hasNew = m_instChanges.has(newTypeKey + "." + instanceId);
            bool hasUpdate = m_instChanges.has(updateTypeKey + "." + instanceId);

            if (hasNew) {
                // Optimization: a new followed by an update becomes a new with the update "payload".
                Hash& newTypeHash = m_instChanges.get<Hash>(newTypeKey);
                newTypeHash.erase(instanceId);
                newTypeHash.merge(instNewUpdateEncoder(instanceId, instanceInfo));
                // No need to update the tracking information as there was only a "payload" overwrite of some
                // existing change.
            } else if (hasUpdate) {
                // Optimization: an existing update not following a new should have its "payload" updated by the
                //               "payload" of the new update.
                Hash& updateTypeHash = m_instChanges.get<Hash>(updateTypeKey);
                updateTypeHash.erase(instanceId);
                updateTypeHash.merge(instNewUpdateEncoder(instanceId, instanceInfo));
                // No need to update the tracking information as there was only a "payload" overwrite of some
                // existing change.
            } else {
                // There's no change for the given instance - no optimization to be applied.
                addChange(InstChangeType::UPDATE, instanceId, instanceInfo);
            }
        }


        void InstanceChangeThrottler::submitInstanceGone(const std::string& instanceId, const Hash& instanceInfo) {
            const std::string& instType = instanceInfo.get<std::string>("type");

            std::lock_guard<std::mutex> lock(m_instChangesMutex);

            std::string newTypeKey = getInstChangeTypeStr(InstChangeType::NEW) + "." + instType;
            std::string updateTypeKey = getInstChangeTypeStr(InstChangeType::UPDATE) + "." + instType;

            bool hasNew = m_instChanges.has(newTypeKey + "." + instanceId);
            bool hasUpdate = m_instChanges.has(updateTypeKey + "." + instanceId);

            if (hasNew) {
                // Optimization: a new followed by a gone is removed and the gone isn't added.
                Hash& newTypeHash = m_instChanges.get<Hash>(newTypeKey);
                newTypeHash.erase(instanceId);
                m_totalChangesInCycle--;
                if (newTypeHash.empty()) {
                    // The second level hash key now references an empty hash - should erase it as well.
                    m_instChanges.erase(newTypeKey);
                }
            }
            if (hasUpdate) {
                // Optimization: an update followed by a gone is removed and the gone isn't added.
                Hash& updateTypeHash = m_instChanges.get<Hash>(updateTypeKey);
                updateTypeHash.erase(instanceId);
                m_totalChangesInCycle--;
                if (updateTypeHash.empty()) {
                    // The second level hash key now references an empty hash - should erase it as well.
                    m_instChanges.erase(updateTypeKey);
                }
            }

            if (!hasNew) {
                // There was no new change for the device - must add the gone change.
                addChange(InstChangeType::GONE, instanceId, instanceInfo);
            }
        }


        void InstanceChangeThrottler::addChange(InstChangeType changeType, const std::string& instanceId,
                                                const Hash& instanceInfo) {
            auto encodeInstanceInfo = [this, changeType, &instanceId, &instanceInfo]() {
                Hash eif;
                switch (changeType) {
                    case InstChangeType::NEW:
                    case InstChangeType::UPDATE:
                        eif = instNewUpdateEncoder(instanceId, instanceInfo);
                        break;
                    case InstChangeType::GONE:
                        eif = Hash(instanceId, Hash());
                        break;
                }
                return eif;
            };

            // Note: It is assumed that this method is always called under protection of m_instChangesMutex.
            const std::string changeTypeStr = getInstChangeTypeStr(changeType);

            Hash& typeLevelHash = m_instChanges.get<Hash>(changeTypeStr);
            const std::string& typeKey = instanceInfo.get<std::string>("type");

            bool addedChange = false;

            // Second level hash always exists; it may be an empty hash though.
            if (typeLevelHash.has(typeKey)) {
                // There's already a third level hash.
                Hash& instanceLevelHash = typeLevelHash.get<Hash>(typeKey);
                // From the optmizations performed while submitting changes, it can be assumed that there will
                // be no collision in here; to be on the safe side though, an error is logged before overwritting.
                if (instanceLevelHash.has(instanceId)) {
                    KARABO_LOG_FRAMEWORK_WARN << "Unexpected collision for change of type '" << changeTypeStr
                                              << "' for instance '" << instanceId << "' of type '" << typeKey << "'. "
                                              << "No instance change data will be overwritten.";
                } else {
                    instanceLevelHash.merge(encodeInstanceInfo());
                    addedChange = true;
                }
            } else {
                // There's no third level hash yet for the given second level; create it.
                const Hash& instanceLevelHash(encodeInstanceInfo());
                typeLevelHash.set(typeKey, instanceLevelHash);
                addedChange = true;
            }

            if (addedChange) {
                m_totalChangesInCycle++;
            }

            if (m_totalChangesInCycle >= m_maxChangesPerCycle) {
                // Maximum number of changes reached - cancels the next scheduled flush (if possible) and
                // flushes immediately.
                if (m_throttlerTimer.cancel() > 0) {
                    flushThrottler();
                }
            }
        }


        //</editor-fold>


        //<editor-fold desc="Throttler cycle">


        unsigned int InstanceChangeThrottler::maxChangesPerCycle() const {
            return m_maxChangesPerCycle;
        }


        unsigned int InstanceChangeThrottler::cycleIntervalMs() const {
            return m_cycleIntervalMs;
        }


        void InstanceChangeThrottler::runThrottlerCycleAsync(const boost::system::error_code& e) {
            if (e) return;

            std::lock_guard<std::mutex> lock(m_instChangesMutex);
            flushThrottler();
        }


        void InstanceChangeThrottler::flush() {
            std::lock_guard<std::mutex> lock(m_instChangesMutex);
            flushThrottler(true);
        }


        void InstanceChangeThrottler::flushThrottler(bool kickNextCycle) {
            // Note: this method assumes it runs under the protection of m_instChangesMutex.

            if (m_instChangeHandler && m_totalChangesInCycle > 0) {
                m_instChangeHandler(m_instChanges);
            }

            if (kickNextCycle) {
                resetCycleInstChanges();
                kickNextThrottlerCycleAsync();
            }
        }

        //</editor-fold>


        //<editor-fold desc="Internal helper methods">


        void InstanceChangeThrottler::kickNextThrottlerCycleAsync() {
            m_throttlerTimer.expires_after(milliseconds(m_cycleIntervalMs));
            m_throttlerTimer.async_wait(
                  bind_weak(&InstanceChangeThrottler::runThrottlerCycleAsync, this, boost::asio::placeholders::error));
        }


        std::string InstanceChangeThrottler::getInstChangeTypeStr(InstChangeType changeType) const {
            std::string changeTypeStr;

            switch (changeType) {
                case InstChangeType::NEW:
                    changeTypeStr = "new";
                    break;
                case InstChangeType::UPDATE:
                    changeTypeStr = "update";
                    break;
                case InstChangeType::GONE:
                    changeTypeStr = "gone";
                    break;
            }

            return changeTypeStr;
        }


        void InstanceChangeThrottler::initCycleInstChanges() {
            m_instChanges.set<Hash>(getInstChangeTypeStr(InstChangeType::NEW), Hash());
            m_instChanges.set<Hash>(getInstChangeTypeStr(InstChangeType::UPDATE), Hash());
            m_instChanges.set<Hash>(getInstChangeTypeStr(InstChangeType::GONE), Hash());
            m_totalChangesInCycle = 0;
        }


        void InstanceChangeThrottler::resetCycleInstChanges() {
            // It is assumed that this method is called under m_instChangesMutex protection or in
            // "thread-safe" occasions during the life cycle of the Throttler - e.g. during construction.
            m_instChanges.get<Hash>(getInstChangeTypeStr(InstChangeType::NEW)).clear();
            m_instChanges.get<Hash>(getInstChangeTypeStr(InstChangeType::UPDATE)).clear();
            m_instChanges.get<Hash>(getInstChangeTypeStr(InstChangeType::GONE)).clear();
            m_totalChangesInCycle = 0;
        }

        //</editor-fold>

    } // namespace core

} // namespace karabo
