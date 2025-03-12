/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 26, 2025, 4:25 PM
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

#include "Device.hh"

#include <unistd.h>

#include <boost/algorithm/string.hpp>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_set>

#include "DeviceClient.hh"
#include "Lock.hh"
#include "karabo/log/Logger.hh"
#include "karabo/log/utils.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/AlarmConditionElement.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/HashFilter.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/OverwriteElement.hh"
#include "karabo/util/RollingWindowStatistics.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/StackTrace.hh"
#include "karabo/util/State.hh"
#include "karabo/util/StateElement.hh"
#include "karabo/util/Timestamp.hh"
#include "karabo/util/Trainstamp.hh"
#include "karabo/util/Validator.hh"
#include "karabo/util/Version.hh"
#include "karabo/xms/InputChannel.hh"
#include "karabo/xms/OutputChannel.hh"
#include "karabo/xms/SlotElement.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {
    namespace core {


        BaseDevice::~BaseDevice() {}


        void BaseDevice::startInitialFunctions() {
            // Call second constructors in the same order as first constructors were called
            for (size_t i = 0; i < m_initialFunc.size(); ++i) m_initialFunc[i]();
        }


        void Device::expectedParameters(karabo::util::Schema& expected) {
            using namespace karabo::util;
            using namespace karabo::xms;

            STRING_ELEMENT(expected)
                  .key("_deviceId_")
                  .displayedName("_DeviceID_")
                  .description("Do not set this property, it will be set by the device-server")
                  .adminAccess()
                  .assignmentInternal()
                  .noDefaultValue()
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("deviceId")
                  .displayedName("DeviceID")
                  .description("The device instance ID uniquely identifies a device instance in the distributed system")
                  .readOnly()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("heartbeatInterval")
                  .displayedName("Heartbeat interval")
                  .description("The heartbeat interval")
                  .assignmentOptional()
                  .defaultValue(120)
                  .minInc(10) // avoid too much traffic - 10 is minimum of server as well
                  .adminAccess()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("_serverId_")
                  .displayedName("_ServerID_")
                  .description("Do not set this property, it will be set by the device-server")
                  .adminAccess()
                  .assignmentInternal()
                  .noDefaultValue()
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("classId")
                  .displayedName("ClassID")
                  .description("The (factory)-name of the class of this device")
                  .readOnly()
                  .initialValue(Device::classInfo().getClassId())
                  .commit();

            STRING_ELEMENT(expected)
                  .key("classVersion")
                  .displayedName("Class version")
                  .description("The version of the class of this device defined in KARABO_CLASSINFO")
                  .expertAccess()
                  .readOnly()
                  // No version dependent initial value:
                  // It would make the static schema version dependent, i.e. introduce fake changes.
                  .commit();

            STRING_ELEMENT(expected)
                  .key("karaboVersion")
                  .displayedName("Karabo version")
                  .description("The version of the Karabo framework running this device")
                  .expertAccess()
                  .readOnly()
                  // No version dependent initial value, see above for "classVersion"
                  .commit();

            STRING_ELEMENT(expected)
                  .key("serverId")
                  .displayedName("ServerID")
                  .description("The device-server on which this device is running on")
                  .expertAccess()
                  .readOnly()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("hostName")
                  .displayedName("Host")
                  .description("Do not set this property, it will be set by the device-server.")
                  .expertAccess()
                  .assignmentInternal()
                  .noDefaultValue()
                  .init()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("pid")
                  .displayedName("Process ID")
                  .description("The unix process ID of the device (i.e. of the server")
                  .expertAccess()
                  .readOnly()
                  .initialValue(0)
                  .commit();

            STATE_ELEMENT(expected)
                  .key("state")
                  .displayedName("State")
                  .description("The current state the device is in")
                  .initialValue(State::UNKNOWN)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("status")
                  .displayedName("Status")
                  .description("A more detailed status description")
                  .readOnly()
                  .initialValue("")
                  .commit();

            ALARM_ELEMENT(expected)
                  .key("alarmCondition")
                  .displayedName("Alarm condition")
                  .description(
                        "The current alarm condition of the device. "
                        "Evaluates to the highest condition on any"
                        " property if not set manually.")
                  .initialValue(AlarmCondition::NONE)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("lockedBy")
                  .displayedName("Locked by")
                  .reconfigurable()
                  .assignmentOptional()
                  .defaultValue("")
                  .setSpecialDisplayType("lockedBy")
                  .commit();

            SLOT_ELEMENT(expected) //
                  .key("slotClearLock")
                  .displayedName("Clear Lock")
                  .expertAccess()
                  .commit();


            STRING_ELEMENT(expected)
                  .key("lastCommand")
                  .displayedName("Last command")
                  .description("The last slot called.")
                  .adminAccess()
                  .readOnly()
                  .initialValue("")
                  .commit();

            NODE_ELEMENT(expected)
                  .key("performanceStatistics")
                  .displayedName("Performance Statistics")
                  .description("Accumulates some statistics")
                  .expertAccess()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("performanceStatistics.messagingProblems")
                  .displayedName("Messaging problems")
                  .description("If true, there is a problem consuming broker messages")
                  .expertAccess()
                  // threshold is exclusive: value true fulfills "> false" and triggers alarm whereas false does not
                  // .alarmHigh(false)
                  // .info("Unreliable broker message consumption - consider restarting device!")
                  // .needsAcknowledging(true)
                  .readOnly()
                  .initialValue(false)
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("performanceStatistics.enable")
                  .displayedName("Enable Performance Indicators")
                  .description("Enables some statistics to follow the performance of an individual device")
                  .reconfigurable()
                  .expertAccess()
                  .assignmentOptional()
                  .defaultValue(false)
                  .commit();

            FLOAT_ELEMENT(expected)
                  .key("performanceStatistics.processingLatency")
                  .displayedName("Processing latency")
                  .description("Average time interval between remote message sending and processing it in this device.")
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  .expertAccess()
                  .readOnly()
                  .initialValue(0.f)
                  .warnHigh(3000.f) // 3 s
                  .info("Long average time between message being sent and start of its processing")
                  .needsAcknowledging(false)
                  .alarmHigh(10000.f) // 10 s
                  .info("Very long average time between message being sent and start of its processing")
                  .needsAcknowledging(false)
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("performanceStatistics.maxProcessingLatency")
                  .displayedName("Maximum latency")
                  .description("Maximum processing latency within averaging interval.")
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  .expertAccess()
                  .readOnly()
                  .initialValue(0)
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("performanceStatistics.numMessages")
                  .displayedName("Number of messages")
                  .description("Number of messages received within averaging interval.")
                  .unit(Unit::COUNT)
                  .expertAccess()
                  .readOnly()
                  .initialValue(0)
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("performanceStatistics.maxEventLoopLatency")
                  .displayedName("Max. event loop latency")
                  .description(
                        "Maximum time interval between posting a message on the central event loop "
                        "and processing it within averaging interval.")
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  .expertAccess()
                  .readOnly()
                  .initialValue(0)
                  .commit();
        }


        Device::Device(const karabo::util::Hash& configuration) : m_lastBrokerErrorStamp(0ull, 0ull) {
            // Set serverId
            if (configuration.has("_serverId_")) configuration.get("_serverId_", m_serverId);
            else m_serverId = KARABO_NO_SERVER;

            // Set instanceId
            if (configuration.has("_deviceId_")) configuration.get("_deviceId_", m_deviceId);
            else m_deviceId = "__none__";

            // Default visibility/accessLevel...
            m_visibility = karabo::util::Schema::OBSERVER;

            // Make the configuration the initial state of the device
            m_parameters = configuration;

            m_timeId = 0;
            m_timeSec = 0;
            m_timeFrac = 0;
            m_timePeriod = 0; // zero as identifier of initial value used in slotTimeTick

            // Setup the validation classes
            karabo::util::Validator::ValidationRules rules;
            rules.allowAdditionalKeys = false;
            rules.allowMissingKeys = true;
            rules.allowUnrootedConfiguration = true;
            rules.injectDefaults = false;
            rules.injectTimestamps = true;
            m_validatorIntern.setValidationRules(rules);
            rules.forceInjectedTimestamp = true; // no externally contributed timestamp!
            m_validatorExtern.setValidationRules(rules);
        }


        Device::~Device() {
            KARABO_LOG_FRAMEWORK_TRACE << "Device::~Device() dtor : m_deviceClient.use_count()="
                                       << m_deviceClient.use_count() << "\n"
                                       << karabo::util::StackTrace();
            m_deviceClient.reset();
        };


        DeviceClient& Device::remote() {
            if (!m_deviceClient) {
                // Initialize an embedded device client (for composition)
                m_deviceClient = std::make_shared<DeviceClient>(shared_from_this(), false);
                m_deviceClient->initialize();
            }
            return *(m_deviceClient);
        }


        void Device::set(const std::string& key, const karabo::util::State& state) {
            set(key, state, getActualTimestamp());
        }


        void Device::set(const std::string& key, const karabo::util::AlarmCondition& condition) {
            set(key, condition, getActualTimestamp());
        }


        void Device::set(const std::string& key, const karabo::util::State& state,
                         const karabo::util::Timestamp& timestamp) {
            karabo::util::Hash h(key, state.name());
            h.setAttribute(key, KARABO_INDICATE_STATE_SET, true);
            set(h, timestamp);
        }


        void Device::set(const std::string& key, const karabo::util::AlarmCondition& condition,
                         const karabo::util::Timestamp& timestamp) {
            karabo::util::Hash h(key, condition.asString());
            h.setAttribute(key, KARABO_INDICATE_ALARM_SET, true);
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            setNoLock(h, timestamp);
            // also set the fields attribute
            m_parameters.setAttribute(key, KARABO_ALARM_ATTR, condition.asString());
        }


        void Device::writeChannel(const std::string& channelName, const karabo::util::Hash& data) {
            this->writeChannel(channelName, data, this->getActualTimestamp());
        }


        void Device::writeChannel(const std::string& channelName, const karabo::util::Hash& data,
                                  const karabo::util::Timestamp& timestamp, bool safeNDArray) {
            using namespace karabo::xms;
            OutputChannel::Pointer channel = this->getOutputChannel(channelName);
            // Provide proper meta data information, as well as correct train- and timestamp
            OutputChannel::MetaData meta(m_instanceId + ":" + channelName, timestamp);
            channel->write(data, meta);
            channel->update(safeNDArray);
        }


        void Device::set(const karabo::util::Hash& hash) {
            this->set(hash, getActualTimestamp());
        }


        void Device::set(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp) {
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            setNoLock(hash, timestamp);
        }


        void Device::setNoLock(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp) {
            using namespace karabo::util;
            std::pair<bool, std::string> result;

            Hash validated;
            result = m_validatorIntern.validate(m_fullSchema, hash, validated, timestamp);

            if (result.first == false) {
                const std::string msg("Bad parameter setting attempted, validation reports: " + result.second);
                KARABO_LOG_WARN << msg;
                throw KARABO_PARAMETER_EXCEPTION(msg);
            }

            if (!validated.empty()) {
                m_parameters.merge(validated, karabo::util::Hash::REPLACE_ATTRIBUTES);

                auto signal = "signalChanged"; // less reliable delivery
                if (validated.has("state") || m_validatorIntern.hasReconfigurableParameter()) {
                    // If Hash contains state or at least one reconfigurable parameter:
                    // ==> more reliable delivery.
                    signal = "signalStateChanged";
                }
                emit(signal, validated, getInstanceId());
            }
        }


        void Device::setNoValidate(const karabo::util::Hash& hash) {
            this->setNoValidate(hash, getActualTimestamp());
        }


        void Device::setNoValidate(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp) {
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            this->setNoValidateNoLock(hash, timestamp);
        }


        void Device::setNoValidateNoLock(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp) {
            using namespace karabo::util;
            if (!hash.empty()) {
                Hash tmp(hash);
                std::vector<std::string> paths;
                tmp.getPaths(paths);

                for (const std::string& path : paths) {
                    timestamp.toHashAttributes(tmp.getAttributes(path));
                }
                m_parameters.merge(tmp, Hash::REPLACE_ATTRIBUTES);

                // Find out which signal to use...
                auto signal = "signalChanged"; // default, less reliable delivery
                if (tmp.has("state")) {
                    // if Hash contains 'state' key -> signalStateChanged
                    signal = "signalStateChanged"; // more reliable delivery
                } else {
                    for (const std::string& path : paths) {
                        if (m_fullSchema.has(path) && m_fullSchema.isAccessReconfigurable(path)) {
                            // if Hash contains at least one reconfigurable parameter -> signalStateChanged
                            signal = "signalStateChanged"; // more reliable delivery
                            break;
                        }
                    }
                }

                // ...and finally emit:
                emit(signal, tmp, getInstanceId());
            }
        }


        karabo::util::Schema Device::getFullSchema() const {
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            return m_fullSchema;
        }


        void Device::appendSchema(const karabo::util::Schema& schema, const bool /*unused*/) {
            KARABO_LOG_DEBUG << "Append Schema requested";
            const karabo::util::Timestamp stamp(getActualTimestamp());
            karabo::util::Hash validated;
            karabo::util::Validator::ValidationRules rules;
            rules.allowAdditionalKeys = true;
            rules.allowMissingKeys = true;
            rules.allowUnrootedConfiguration = true;
            rules.injectDefaults = true;
            rules.injectTimestamps = false; // add later when set(validated) is called
            karabo::util::Validator v(rules);
            // Set default values for all parameters in appended Schema
            v.validate(schema, karabo::util::Hash(), validated);
            {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);

                // Take care of OutputChannels schema changes - have to recreate to make other end aware
                std::unordered_set<std::string> outChannelsToRecreate;
                for (const auto& path : getOutputChannelNames()) {
                    if (m_fullSchema.has(path) && schema.has(path) &&
                        (!schema.hasDisplayType(path) || schema.getDisplayType(path) != "OutputChannel")) {
                        // potential output schema change without using OUTPUT_CHANNEL
                        outChannelsToRecreate.insert(path);
                    }
                    // else if (schema.getDisplayType(path) == "OutputChannel"):
                    //    will be recreated by initChannels(schema) below
                }

                // Clear cache
                m_stateDependentSchema.clear();

                // Save injected
                m_injectedSchema.merge(schema);

                // Stores leaves in current full_schema to avoid sending them again later.
                // Empty nodes must not enter here since otherwise leaves injected into them would not be sent,
                // either.
                std::vector<std::string> prevFullSchemaLeaves = m_fullSchema.getPaths();
                const auto itEnd = std::remove_if(prevFullSchemaLeaves.begin(), prevFullSchemaLeaves.end(),
                                                  [this](const std::string& p) { return m_fullSchema.isNode(p); });
                prevFullSchemaLeaves.resize(itEnd - prevFullSchemaLeaves.begin()); // remove_if did not alter length

                // Merge to full schema
                m_fullSchema.merge(m_injectedSchema);

                // Notify the distributed system
                emit("signalSchemaUpdated", m_fullSchema, m_deviceId);

                // Keep new leaves only (to avoid re-sending updates with the same values), i.e.
                // removes all leaves from validated that are in previous full schema.
                for (const std::string& p : prevFullSchemaLeaves) {
                    validated.erasePath(p);
                }

                setNoLock(validated, stamp);

                // Init any freshly injected channels
                initChannels(schema);
                // ... and those output channels with potential Schema change
                for (const std::string& outToCreate : outChannelsToRecreate) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "appendSchema triggers creation of output channel '" << outToCreate
                                               << "'";
                    prepareOutputChannel(outToCreate);
                }
            }

            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Schema appended";
        }


        void Device::updateSchema(const karabo::util::Schema& schema, const bool /*unused*/) {
            KARABO_LOG_DEBUG << "Update Schema requested";
            karabo::util::Hash validated;
            karabo::util::Validator::ValidationRules rules;
            rules.allowAdditionalKeys = true;
            rules.allowMissingKeys = true;
            rules.allowUnrootedConfiguration = true;
            rules.injectDefaults = true;
            rules.injectTimestamps = false; // Will do later when set(validated,..) is called
            karabo::util::Validator v(rules);
            const karabo::util::Timestamp stamp(getActualTimestamp());
            v.validate(schema, karabo::util::Hash(), validated);
            {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                // Clear previously injected parameters.
                // But not blindly all paths (not only keys!) from m_injectedSchema: injection might have been done
                // to update attributes like alarm levels, min/max values, size, etc. of existing properties.
                for (const std::string& path : m_injectedSchema.getPaths()) {
                    if (!(m_staticSchema.has(path) || schema.has(path))) {
                        m_parameters.erasePath(path);
                        // Now we might have removed 'n.m.l.c' completely although 'n.m' is in static schema:
                        // need to restore (empty) node 'n.m':
                        size_t pos = path.rfind(util::Hash::k_defaultSep); // Last dot to cut path
                        while (pos != std::string::npos) {
                            const std::string& p =
                                  path.substr(0, pos); // first 'n.m.l', then 'n.m' (without break below then 'n')
                            if (m_staticSchema.has(p) && !m_parameters.has(p)) {
                                m_parameters.set(p, karabo::util::Hash());
                                break; // 'n.m' added added back (after 'n.m.l' failed)
                            }
                            pos = p.rfind(util::Hash::k_defaultSep);
                        }
                    }
                }

                // Clear cache
                m_stateDependentSchema.clear();

                // Stores leaves in current full_schema to avoid sending them again later.
                // Empty nodes must not enter here since otherwise leaves injected into them would not be sent,
                // either.
                std::vector<std::string> prevFullSchemaLeaves = m_fullSchema.getPaths();
                const auto itEnd = std::remove_if(prevFullSchemaLeaves.begin(), prevFullSchemaLeaves.end(),
                                                  [this](const std::string& p) { return m_fullSchema.isNode(p); });
                prevFullSchemaLeaves.resize(itEnd - prevFullSchemaLeaves.begin()); // remove_if did not alter length

                // Erase any previously injected InputChannels
                for (const auto& inputNameChannel : getInputChannels()) {
                    const std::string& path = inputNameChannel.first;
                    // Do not touch static InputChannel (even if injected again to change some properties)
                    if (m_staticSchema.has(path)) continue;
                    if (m_injectedSchema.has(path)) {
                        removeInputChannel(path);
                    }
                }

                // Take care of OutputChannels:
                // Remove or take care of schema changes - in latter case, have to recreate to make other end aware
                std::unordered_set<std::string> outChannelsToRecreate;
                for (const auto& path : getOutputChannelNames()) {
                    if (m_injectedSchema.has(path)) {
                        if (m_staticSchema.has(path)) {
                            // Channel changes its schema back to its default
                            outChannelsToRecreate.insert(path);
                        } else {
                            // Previously injected channel has to be removed
                            KARABO_LOG_FRAMEWORK_INFO << "updateSchema: Remove output channel '" << path << "'";
                            removeOutputChannel(path);
                        }
                    }
                    if (m_staticSchema.has(path) && schema.has(path) &&
                        (!schema.hasDisplayType(path) || schema.getDisplayType(path) != "OutputChannel")) {
                        // potential output schema change without using OUTPUT_CHANNEL
                        outChannelsToRecreate.insert(path);
                    }
                    // else if (schema.getDisplayType(path) == "OutputChannel"):
                    //    will be recreated by initChannels(m_injectedSchema) below
                }

                // Resets fullSchema
                m_fullSchema = m_staticSchema;

                // Save injected
                m_injectedSchema = schema;

                // Merge to full schema
                m_fullSchema.merge(m_injectedSchema);

                // Notify the distributed system
                emit("signalSchemaUpdated", m_fullSchema, m_deviceId);

                // Keep new leaves only (to avoid re-sending updates with the same values), i.e.
                // removes all leaves from validated that are in previous full schema.
                for (const std::string& p : prevFullSchemaLeaves) {
                    validated.erasePath(p);
                }
                setNoLock(validated, stamp);

                // Init any freshly injected channels
                initChannels(m_injectedSchema);
                // ... and those with potential Schema change
                for (const std::string& outToCreate : outChannelsToRecreate) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "updateSchema triggers creation of output channel '" << outToCreate
                                               << "'";
                    prepareOutputChannel(outToCreate);
                }
            }

            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Schema updated";
        }


        bool Device::keyHasAlias(const std::string& key) const {
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            return m_fullSchema.keyHasAlias(key);
        }


        karabo::util::Types::ReferenceType Device::getValueType(const std::string& key) const {
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            return m_fullSchema.getValueType(key);
        }


        karabo::util::Hash Device::getCurrentConfiguration(const std::string& tags) const {
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            if (tags.empty()) return m_parameters;
            karabo::util::Hash filtered;
            karabo::util::HashFilter::byTag(m_fullSchema, m_parameters, filtered, tags);
            return filtered;
        }


        karabo::util::Hash Device::getCurrentConfigurationSlice(const std::vector<std::string>& paths) const {
            karabo::util::Hash result;

            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            for (const std::string& path : paths) {
                const karabo::util::Hash::Node& node = m_parameters.getNode(path);
                // Copy value and attributes
                karabo::util::Hash::Node& newNode = result.set(path, node.getValueAsAny());
                newNode.setAttributes(node.getAttributes());
            }
            return result;
        }


        karabo::util::Hash Device::filterByTags(const karabo::util::Hash& hash, const std::string& tags) const {
            karabo::util::Hash filtered;
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            karabo::util::HashFilter::byTag(m_fullSchema, hash, filtered, tags);
            return filtered;
        }


        void Device::updateState(const karabo::util::State& currentState, karabo::util::Hash other,
                                 const karabo::util::Timestamp& timestamp) {
            try {
                const std::string& stateName = currentState.name();
                KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << ".updateState: \"" << stateName << "\".";
                if (getState() != currentState) {
                    // Set state as string, but add state marker attribute KARABO_INDICATE_STATE_SET
                    other.set("state", stateName).setAttribute(KARABO_INDICATE_STATE_SET, true);
                    // Compare with state enum
                    if (currentState == karabo::util::State::ERROR) {
                        updateInstanceInfo(karabo::util::Hash("status", "error"));
                    } else if (currentState == karabo::util::State::UNKNOWN) {
                        updateInstanceInfo(karabo::util::Hash("status", "unknown"));
                    } else {
                        // Reset the error status - protect against non-initialised instanceInfo
                        const karabo::util::Hash info(getInstanceInfo());
                        if (!info.has("status") || info.get<std::string>("status") == "error" ||
                            info.get<std::string>("status") == "unknown") {
                            updateInstanceInfo(karabo::util::Hash("status", "ok"));
                        }
                    }
                }
                if (!other.empty()) set(other, timestamp);

                // Place the new state as reply for interested event initiators
                // This is intended for lazy device programmers: A slot call should reply any new state. If the
                // slot does not call reply(..) again after updateState, the slot will implicitly take this here.
                // Note that in case of asynchronous slot completion the AsyncReply has to be instructed explicitly.
                reply(stateName);

            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW
            }
        }


        void Device::setAlarmCondition(const karabo::util::AlarmCondition& condition, bool needsAcknowledging,
                                       const std::string& description) {
            using namespace karabo::util;

            const Timestamp timestamp(getActualTimestamp());
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            Hash h;
            h.set("alarmCondition", condition.asString()).setAttribute(KARABO_INDICATE_ALARM_SET, true);
            // also set the fields attribute to this condition
            this->setNoValidateNoLock(h, timestamp);
            this->m_parameters.setAttribute("alarmCondition", KARABO_ALARM_ATTR, condition.asString());
        }


        const karabo::util::AlarmCondition& Device::getAlarmCondition(const std::string& key,
                                                                      const std::string& sep) const {
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            const std::string& propertyCondition =
                  this->m_parameters.template getAttribute<std::string>(key, KARABO_ALARM_ATTR, sep.at(0));
            return karabo::util::AlarmCondition::fromString(propertyCondition);
        }


        void Device::slotTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                  unsigned long long period) {
            {
                std::lock_guard<std::mutex> lock(m_timeChangeMutex);
                m_timeId = id;
                m_timeSec = sec;
                m_timeFrac = frac;
                m_timePeriod = period;
            }
            onTimeTick(id, sec, frac, period);
        }


        void Device::appendSchemaMaxSize(const std::string& path, unsigned int value, bool emitFlag) {
            using karabo::util::OVERWRITE_ELEMENT;
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            if (!m_fullSchema.has(path)) {
                throw KARABO_PARAMETER_EXCEPTION("Path \"" + path + "\" not found in the device schema.");
            }
            m_stateDependentSchema.clear();
            // Do not touch static schema - that must be restorable via updateSchema(Schema())
            // OVERWRITE_ELEMENT checks whether max size attribute makes sense for path
            OVERWRITE_ELEMENT(m_fullSchema).key(path).setNewMaxSize(value).commit();
            if (m_injectedSchema.has(path)) {
                OVERWRITE_ELEMENT(m_injectedSchema).key(path).setNewMaxSize(value).commit();
            }

            // Notify the distributed system if needed
            if (emitFlag) emit("signalSchemaUpdated", m_fullSchema, m_deviceId);
        }


        karabo::util::Timestamp Device::getTimestamp(const karabo::util::Epochstamp& epoch) const {
            unsigned long long id = 0;
            {
                std::lock_guard<std::mutex> lock(m_timeChangeMutex);
                if (m_timePeriod > 0) {
                    const karabo::util::Epochstamp epochLastReceived(m_timeSec, m_timeFrac);
                    // duration is always positive, irrespective whether epoch or epochLastReceived is more recent
                    const karabo::util::TimeDuration duration = epoch.elapsed(epochLastReceived);
                    const unsigned long long nPeriods =
                          (duration.getTotalSeconds() * 1000000ull + duration.getFractions(karabo::util::MICROSEC)) /
                          m_timePeriod;
                    if (epochLastReceived <= epoch) {
                        id = m_timeId + nPeriods;
                    } else if (m_timeId >= nPeriods + 1ull) { // sanity check
                        id = m_timeId - nPeriods - 1ull;
                    } else {
                        KARABO_LOG_FRAMEWORK_WARN << "Bad input: (train)Id zero since epoch = " << epoch.toIso8601()
                                                  << "; from time server: epoch = " << epochLastReceived.toIso8601()
                                                  << ", id = " << m_timeId << ", period = " << m_timePeriod << " mus";
                    }
                }
            }
            return karabo::util::Timestamp(epoch, karabo::util::Trainstamp(id));
        }


        void Device::finalizeInternalInitialization(const karabo::net::Broker::Pointer& connection,
                                                    bool consumeBroadcasts, const std::string& timeServerId) {
            using namespace karabo::util;
            using namespace karabo::net;
            using namespace std::placeholders;

            //
            // First set all parameters (which could not yet be set in constructor) and init the SignalSlotable
            //

            // These initializations or done here and not in the constructor
            // as they involve virtual function calls
            this->initClassId();
            this->initSchema();

            bool hasAvailableScenes = false;
            bool hasAvailableMacros = false;
            bool hasAvailableInterfaces = false;

            m_timeServerId = timeServerId;

            int heartbeatInterval = 0;
            {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                // ClassId
                m_parameters.set("classId", m_classId);
                m_parameters.set("classVersion", getClassInfo().getVersion());
                m_parameters.set("karaboVersion", karabo::util::Version::getVersion());
                // DeviceId
                m_parameters.set("deviceId", m_deviceId);
                // ServerId
                m_parameters.set("serverId", m_serverId);
                // ProcessId
                m_parameters.set("pid", ::getpid());
                // Set hostname if missing
                if (!m_parameters.has("hostName")) {
                    m_parameters.set("hostName", net::bareHostName());
                }
                // The following lines of code are needed to initially inject timestamps to the parameters
                karabo::util::Hash validated;
                std::pair<bool, std::string> result =
                      m_validatorIntern.validate(m_fullSchema, m_parameters, validated, getActualTimestamp());
                if (result.first == false) {
                    KARABO_LOG_WARN << "Bad parameter setting attempted, validation reports: " << result.second;
                }
                m_parameters.merge(validated, karabo::util::Hash::REPLACE_ATTRIBUTES);

                // Do this under mutex protection
                hasAvailableScenes = m_parameters.has("availableScenes");
                hasAvailableMacros = m_parameters.has("availableMacros");
                hasAvailableInterfaces = m_parameters.has("interfaces");

                heartbeatInterval = m_parameters.get<int>("heartbeatInterval");
            }

            // Prepare some info further describing this particular instance
            karabo::util::Hash instanceInfo;
            instanceInfo.set("type", "device");
            instanceInfo.set("classId", getClassInfo().getClassId());
            instanceInfo.set("serverId", m_serverId);
            instanceInfo.set("visibility", m_visibility);
            instanceInfo.set("host", this->get<std::string>("hostName"));
            std::string status;
            const karabo::util::State state = this->getState();
            if (state == State::ERROR) {
                status = "error";
            } else if (state == State::UNKNOWN) {
                status = "unknown";
            } else {
                status = "ok";
            }
            instanceInfo.set("status", status);

            // the capabilities field specifies the optional capabilities a device provides.
            unsigned int capabilities = 0;

            if (hasAvailableScenes) capabilities |= Capabilities::PROVIDES_SCENES;
            if (hasAvailableMacros) capabilities |= Capabilities::PROVIDES_MACROS;
            if (hasAvailableInterfaces) capabilities |= Capabilities::PROVIDES_INTERFACES;

            instanceInfo.set("capabilities", capabilities);

            if (hasAvailableInterfaces) {
                unsigned int interfaces = 0;
                const std::vector<std::string>& availableInterfaces = get<std::vector<std::string>>("interfaces");
                for (const std::string& desc : availableInterfaces)
                    if (desc == "Motor") interfaces |= Interfaces::Motor;
                    else if (desc == "MultiAxisMotor") interfaces |= Interfaces::MultiAxisMotor;
                    else if (desc == "Trigger") interfaces |= Interfaces::Trigger;
                    else if (desc == "Camera") interfaces |= Interfaces::Camera;
                    else if (desc == "Processor") interfaces |= Interfaces::Processor;
                    else if (desc == "DeviceInstantiator") interfaces |= Interfaces::DeviceInstantiator;
                    else {
                        throw KARABO_LOGIC_EXCEPTION("Provided interface is not supported: " + desc);
                    }
                instanceInfo.set("interfaces", interfaces);
            }

            init(m_deviceId, connection, heartbeatInterval, instanceInfo, consumeBroadcasts);

            //
            // Now do all registrations etc. (Note that it is safe to register slots in the constructor)
            //

            // Initialize Device slots
            this->initDeviceSlots();

            // Register guard for slot calls
            this->registerSlotCallGuardHandler(std::bind(&karabo::core::Device::slotCallGuard, this, _1, _2));

            // Register updateLatencies handler -
            // bind_weak not needed since (and as long as...) handler will not be posted on event loop
            this->registerPerformanceStatisticsHandler(std::bind(&karabo::core::Device::updateLatencies, this, _1));

            // Register message consumption error handler - bind_weak not needed as above
            this->registerBrokerErrorHandler(std::bind(&karabo::core::Device::onBrokerError, this, _1));

            // Instantiate all channels - needs mutex
            {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                this->initChannels(m_fullSchema);
            }
            //
            // Then start SignalSlotable: communication (incl. system registration) starts and thus parallelism!
            //
            SignalSlotable::start();

            KARABO_LOG_FRAMEWORK_INFO << "'" << m_classId << "' (version '" << this->get<std::string>("classVersion")
                                      << "') with deviceId: '" << this->getInstanceId() << "' got started"
                                      << " on server '" << this->getServerId() << "'.";

            //
            // Finally do everything that requires full participation in the system
            //

            // Connect input channels - requires SignalSlotable to be started
            this->connectInputChannels(boost::system::error_code());

            // Start the state machine (call initialization methods in case of noFsm)
            // Do that on the event loop since any blocking should not influence the success of the instantiation
            // procedure, i.e. the device server slot that starts the device should reply immediately, irrespective
            // of what startFsm() does. We wrap the call to startFsm() to treat exceptions.
            net::EventLoop::getIOService().post(util::bind_weak(&Device::wrapStartFsm, this));
        }


        void Device::wrapStartFsm() {
            try {
                this->startInitialFunctions(); // (This function must be inherited from the templated base class
                                               // (it's a concept!)
            } catch (const std::exception& e) {
                const std::string exceptionTxt(e.what());
                KARABO_LOG_ERROR << "The instance with deviceId " << this->getInstanceId()
                                 << " is going down due to an exception in initialization ..." << exceptionTxt;
                // Indicate in the status and kill the device
                set("status", std::string("Initialization failed: ") += exceptionTxt);
                this->call("", "slotKillDevice");
            }
        }


        void Device::initSchema() {
            using namespace karabo::util;
            const Schema staticSchema = getSchema(m_classId, Schema::AssemblyRules(INIT | WRITE | READ));
            {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                // The static schema is the regular schema as assembled by parsing the expectedParameters functions
                m_staticSchema = staticSchema; // Here we lack a Schema::swap(..)...
                // At startup the static schema is identical with the runtime schema
                m_fullSchema = m_staticSchema;
            }
        }


        void Device::initDeviceSlots() {
            using namespace std;

            KARABO_SIGNAL("signalChanged", karabo::util::Hash /*configuration*/, string /*deviceId*/);

            KARABO_SYSTEM_SIGNAL("signalStateChanged", karabo::util::Hash /*configuration*/, string /*deviceId*/);

            KARABO_SYSTEM_SIGNAL("signalSchemaUpdated", karabo::util::Schema /*deviceSchema*/, string /*deviceId*/);

            KARABO_SLOT(slotReconfigure, karabo::util::Hash /*reconfiguration*/)

            KARABO_SLOT(slotGetConfiguration)

            KARABO_SLOT(slotGetConfigurationSlice, karabo::util::Hash)

            KARABO_SLOT(slotGetSchema, bool /*onlyCurrentState*/);

            KARABO_SLOT(slotKillDevice)

            KARABO_SLOT(slotClearLock);

            KARABO_SLOT(slotGetTime, karabo::util::Hash /* UNUSED */);

            KARABO_SLOT(slotGetSystemInfo, karabo::util::Hash /* UNUSED */);
        }


        void Device::initChannels(const karabo::util::Schema& schema, const std::string& topLevel) {
            // Keys under topLevel, without leading "topLevel.":
            const std::vector<std::string>& subKeys = schema.getKeys(topLevel);

            for (const std::string& subKey : subKeys) {
                // Assemble full path out of topLevel and subKey
                const std::string key(topLevel.empty() ? subKey : (topLevel + util::Hash::k_defaultSep) += subKey);
                if (schema.hasDisplayType(key)) {
                    const std::string& displayType = schema.getDisplayType(key);
                    if (displayType == "OutputChannel") {
                        prepareOutputChannel(key);
                    } else if (displayType == "InputChannel") {
                        prepareInputChannel(key);
                    } else {
                        KARABO_LOG_FRAMEWORK_TRACE << "'" << this->getInstanceId() << "' does not create "
                                                   << "in-/output channel for '" << key << "' since it's a '"
                                                   << displayType << "'";
                    }
                } else if (schema.isNode(key)) {
                    // Recursive call going down the tree for channels within nodes
                    KARABO_LOG_FRAMEWORK_TRACE << "'" << this->getInstanceId() << "' looks for input/output "
                                               << "channels under node \"" << key << "\"";
                    initChannels(schema, key);
                }
            }
        }


        void Device::prepareOutputChannel(const std::string& path) {
            KARABO_LOG_FRAMEWORK_INFO << "'" << this->getInstanceId() << "' creates output channel '" << path << "'";
            try {
                karabo::xms::OutputChannel::Pointer channel = createOutputChannel(path, m_parameters);
                if (!channel) {
                    KARABO_LOG_FRAMEWORK_ERROR << "*** 'createOutputChannel' for channel name '" << path
                                               << "' failed to create output channel";
                } else {
                    Device::WeakPointer weakThis(std::dynamic_pointer_cast<Device>(shared_from_this()));
                    channel->registerShowConnectionsHandler(
                          [weakThis, path](const std::vector<karabo::util::Hash>& connections) {
                              Device::Pointer self(weakThis.lock());
                              if (self) self->set(path + ".connections", connections);
                          });
                    channel->registerShowStatisticsHandler([weakThis, path](const std::vector<unsigned long long>& rb,
                                                                            const std::vector<unsigned long long>& wb) {
                        Device::Pointer self(weakThis.lock());
                        if (self) {
                            karabo::util::Hash h(path + ".bytesRead", rb, path + ".bytesWritten", wb);
                            self->set(h);
                        }
                    });
                    karabo::util::Hash update(path, channel->getInitialConfiguration());
                    // do not lock since this method is called under m_objectStateChangeMutex
                    setNoLock(update, getActualTimestamp());
                }
            } catch (const karabo::util::NetworkException& e) {
                KARABO_LOG_ERROR << e.detailedMsg();
            }
        }


        void Device::prepareInputChannel(const std::string& path) {
            KARABO_LOG_FRAMEWORK_INFO << "'" << this->getInstanceId() << "' creates input channel '" << path << "'";
            using karabo::xms::InputChannel;
            using namespace std::placeholders;
            InputChannel::Handlers handlers;
            // If there was already an InputChannel, "rescue" any registered handlers for new channel
            InputChannel::Pointer channel = getInputChannelNoThrow(path);
            if (channel) {
                handlers = channel->getRegisteredHandlers();
            }
            channel = createInputChannel(path, m_parameters, handlers.dataHandler, handlers.inputHandler,
                                         handlers.eosHandler,
                                         util::bind_weak(&Device::trackInputChannelConnections, this, path, _1, _2));
            if (!channel) {
                KARABO_LOG_FRAMEWORK_ERROR << "*** 'createInputChannel' for channel name '" << path
                                           << "' failed to create input channel";
            } else {
                // Set configured connections as missing for now
                // NOTE: Setting ".missingConnections" here and in trackInputChannelConnections(...) (registered as
                // status tracker
                //       above) cannot interfere since here we already have locked m_objectStateChangeMutex that is
                //       also required by the setVectorUpdate(..) inside trackInputChannelConnections(...).
                const util::Hash h(path + ".missingConnections",
                                   m_parameters.get<std::vector<std::string>>(path + ".connectedOutputChannels"));
                setNoLock(h, getActualTimestamp());
            }
        }


        void Device::trackInputChannelConnections(const std::string& inputChannel, const std::string& outputChannel,
                                                  karabo::net::ConnectionStatus status) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Input channel '" << inputChannel << "': connection status for '"
                                       << outputChannel << "' changed: " << static_cast<int>(status);
            if (status == karabo::net::ConnectionStatus::CONNECTING ||
                status == karabo::net::ConnectionStatus::DISCONNECTING) {
                // Ignore any intermediate connection status
                return;
            }

            const VectorUpdate type = (status == karabo::net::ConnectionStatus::DISCONNECTED ? VectorUpdate::addIfNotIn
                                                                                             : VectorUpdate::removeOne);
            setVectorUpdate(inputChannel + ".missingConnections", std::vector<std::string>({outputChannel}), type,
                            getActualTimestamp());
        }


        void Device::slotCallGuard(const std::string& slotName, const std::string& callee) {
            using namespace karabo::util;

            // Check whether the slot is mentioned in the expectedParameters
            // as the call guard only works on those and will ignore all others
            bool isSchemaSlot = false;
            {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                isSchemaSlot = m_fullSchema.has(slotName);
            }

            // Check whether the slot can be called given the current locking state
            const bool lockableSlot = isSchemaSlot || slotName == "slotReconfigure";
            if (allowLock() && lockableSlot && slotName != "slotClearLock") {
                ensureSlotIsValidUnderCurrentLock(slotName, callee);
            }

            // Check whether the slot can be called given the current device state
            if (isSchemaSlot) {
                ensureSlotIsValidUnderCurrentState(slotName);
            }

            // Log the call of this slot by setting a parameter of the device
            if (lockableSlot) {
                std::stringstream source;
                source << slotName << " <- " << callee;
                set("lastCommand", source.str());
            }
        }


        void Device::ensureSlotIsValidUnderCurrentLock(const std::string& slotName, const std::string& callee) {
            const std::string lockHolder = get<std::string>("lockedBy");
            if (!lockHolder.empty()) {
                KARABO_LOG_FRAMEWORK_DEBUG << "'" << getInstanceId() << "' is locked by " << lockHolder
                                           << " and called by '" << callee << "'";
                if (callee != "unknown" && callee != lockHolder) {
                    std::ostringstream msg;
                    msg << "Command \"" << slotName << "\" is not allowed as device is locked by \"" << lockHolder
                        << "\".";
                    throw KARABO_LOCK_EXCEPTION(msg.str());
                }
            }
        }


        void Device::ensureSlotIsValidUnderCurrentState(const std::string& slotName) {
            std::vector<karabo::util::State> allowedStates;
            {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                if (m_fullSchema.hasAllowedStates(slotName)) {
                    allowedStates = m_fullSchema.getAllowedStates(slotName);
                }
            }
            if (!allowedStates.empty()) {
                const karabo::util::State currentState = getState();
                if (std::find(allowedStates.begin(), allowedStates.end(), currentState) == allowedStates.end()) {
                    std::ostringstream msg;
                    msg << "Command \"" << slotName << "\" is not allowed in current state \"" << currentState.name()
                        << "\" of device \"" << m_deviceId << "\".";
                    throw KARABO_LOGIC_EXCEPTION(msg.str());
                }
            }
        }


        void Device::slotGetConfiguration() {
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            reply(m_parameters, m_deviceId);
        }


        void Device::slotGetConfigurationSlice(const karabo::util::Hash& info) {
            const auto& paths = info.get<std::vector<std::string>>("paths");
            reply(getCurrentConfigurationSlice(paths));
        }


        void Device::slotGetSchema(bool onlyCurrentState) {
            if (onlyCurrentState) {
                const karabo::util::State& currentState = getState();
                const karabo::util::Schema schema(getStateDependentSchema(currentState));
                reply(schema, m_deviceId);
            } else {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                reply(m_fullSchema, m_deviceId);
            }
        }


        void Device::slotReconfigure(const karabo::util::Hash& newConfiguration) {
            if (newConfiguration.empty()) return;

            karabo::util::Hash validated;
            std::pair<bool, std::string> result = validate(newConfiguration, validated);

            if (result.first == true) { // is a valid reconfiguration

                // Give device-implementer a chance to specifically react on reconfiguration event by
                // polymorphically calling back
                preReconfigure(validated);

                // nothing to do if empty after preReconfigure
                if (!validated.empty()) {
                    // Merge reconfiguration with current state
                    applyReconfiguration(validated);
                }
                // post reconfigure action
                this->postReconfigure();

            } else { // not a valid reconfiguration
                throw KARABO_PARAMETER_EXCEPTION(result.second);
            }
        }


        std::pair<bool, std::string> Device::validate(const karabo::util::Hash& unvalidated,
                                                      karabo::util::Hash& validated) {
            // Retrieve the current state of the device instance
            const karabo::util::State& currentState = getState();
            const karabo::util::Schema whiteList(getStateDependentSchema(currentState));
            KARABO_LOG_DEBUG << "Incoming (un-validated) reconfiguration:\n" << unvalidated;
            std::pair<bool, std::string> valResult =
                  m_validatorExtern.validate(whiteList, unvalidated, validated, getActualTimestamp());
            KARABO_LOG_DEBUG << "Validated reconfiguration:\n" << validated;
            return valResult;
        }


        void Device::applyReconfiguration(const karabo::util::Hash& reconfiguration) {
            {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                m_parameters.merge(reconfiguration);
            }
            KARABO_LOG_DEBUG << "After user interaction:\n" << reconfiguration;
            if (m_validatorExtern.hasReconfigurableParameter())
                emit("signalStateChanged", reconfiguration, getInstanceId());
            else emit("signalChanged", reconfiguration, getInstanceId());
        }


        void Device::slotKillDevice() {
            // It is important to know who gave us the kill signal
            std::string senderId = getSenderInfo("slotKillDevice")->getInstanceIdOfSender();
            this->preDestruction();       // Give devices a chance to react
            if (senderId == m_serverId) { // Our server killed us
                KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << " is going down as instructed by server";
            } else { // Someone else wants to see us dead, we should inform our server
                KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << " is going down as instructed by \"" << senderId
                                          << "\"";
                call(m_serverId, "slotDeviceGone", m_deviceId);
            }
        }


        karabo::util::Schema Device::getStateDependentSchema(const karabo::util::State& state) {
            using namespace karabo::util;
            const std::string& currentState = state.name();
            KARABO_LOG_FRAMEWORK_DEBUG << "call: getStateDependentSchema() for state: " << currentState;
            // Check cache whether a special state-dependent Schema was created before
            std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
            std::map<std::string, Schema>::iterator it = m_stateDependentSchema.find(currentState);
            if (it == m_stateDependentSchema.end()) { // No
                const Schema::AssemblyRules rules(WRITE, currentState);
                it = m_stateDependentSchema.insert(make_pair(currentState,
                                                             m_fullSchema.subSchemaByRules(rules)))
                           .first; // New one
                KARABO_LOG_FRAMEWORK_DEBUG << "Providing freshly cached state-dependent schema:\n" << it->second;
            } else {
                KARABO_LOG_FRAMEWORK_DEBUG << "Schema was already cached";
            }
            return it->second;
        }


        void Device::updateLatencies(const karabo::util::Hash::Pointer& performanceMeasures) {
            if (this->get<bool>("performanceStatistics.enable")) {
                // Keys and values of 'performanceMeasures' are defined in
                // SignalSlotable::updatePerformanceStatistics and expectedParameters has to foresee this content
                // under node "performanceStatistics".
                this->set(karabo::util::Hash("performanceStatistics", *performanceMeasures));
            }
        }


        void Device::onBrokerError(const std::string& message) {
            // Trigger alarm, but not always a new one (system is busy anyway). By setting messagingProblems
            // up to every second, we can investigate roughly the time of problems via the data logger.
            // Similarly, log to network only every second.
            if (!get<bool>("performanceStatistics.messagingProblems") ||
                (karabo::util::Epochstamp() - m_lastBrokerErrorStamp).getTotalSeconds() >= 1ull) {
                set(karabo::util::Hash("performanceStatistics.messagingProblems", true));
                m_lastBrokerErrorStamp.now();
                KARABO_LOG_ERROR << "Broker consumption problem: " << message;
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << ": Broker consumption problem: " << message;
            }
        }

        karabo::util::Hash Device::getTimeInfo() {
            using namespace karabo::util;
            Hash result;

            Hash::Node& node = result.set("time", true);
            const Timestamp stamp(getActualTimestamp());
            stamp.toHashAttributes(node.getAttributes());

            result.set("timeServerId", m_timeServerId.empty() ? "None" : m_timeServerId);

            Hash::Node& refNode = result.set("reference", true);
            auto& attrs = refNode.getAttributes();
            {
                std::lock_guard<std::mutex> lock(m_timeChangeMutex);
                const Epochstamp epoch(m_timeSec, m_timeFrac);
                const Trainstamp train(m_timeId);
                const Timestamp stamp(epoch, train);
                stamp.toHashAttributes(attrs);
            }
            return result;
        }


        void Device::slotGetSystemInfo(const karabo::util::Hash& /* unused */) {
            using namespace karabo::util;
            Hash result("timeInfo", this->getTimeInfo());
            result.set("broker", m_connection->getBrokerUrl());
            auto user = getlogin(); // Little caveat: getlogin() is a Linux function
            result.set("user", (user ? user : "none"));
            reply(result);
        }

    } // namespace core
} // namespace karabo
