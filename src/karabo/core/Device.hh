/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
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

#ifndef KARABO_CORE_DEVICE_HH
#define KARABO_CORE_DEVICE_HH

#include <unistd.h>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <string>
#include <tuple>
#include <unordered_set>

#include "DeviceClient.hh"
#include "Lock.hh"
#include "NoFsm.hh"
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

        // Convenient logging
#define KARABO_LOG_DEBUG karabo::log::LoggerStream(this->getInstanceId(), spdlog::level::debug)
#define KARABO_LOG_INFO karabo::log::LoggerStream(this->getInstanceId(), spdlog::level::info)
#define KARABO_LOG_WARN karabo::log::LoggerStream(this->getInstanceId(), spdlog::level::warn)
#define KARABO_LOG_ERROR karabo::log::LoggerStream(this->getInstanceId(), spdlog::level::err)

#define KARABO_NO_SERVER "__none__"

        enum Capabilities {

            PROVIDES_SCENES = (1u << 0),
            PROVIDES_MACROS = (1u << 1),
            PROVIDES_INTERFACES = (1u << 2),
            // add future capabilities as bitmask:
            // SOME_OTHER_FUTURE_CAPABILITY = (1u << 3),
        };

        enum Interfaces {

            Motor = (1u << 0),
            MultiAxisMotor = (1u << 1),
            Trigger = (1u << 2),
            Camera = (1u << 3),
            Processor = (1u << 4),
            DeviceInstantiator = (1u << 5),

            // add future interfaces as bitmask:
            // SOME_OTHER_INTERFACE = (1u << 6),
        };


        /**
         * @class BaseDevice
         * @brief The BaseDevice class provides for methods which are template
         *        independent in the Device class
         *
         */
        class BaseDevice : public virtual karabo::xms::SignalSlotable {
           public:
            KARABO_CLASSINFO(BaseDevice, "BaseDevice", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS;

            virtual ~BaseDevice() {}

            /**
             * This method is called to finalize initialization of a device. It is needed to allow user
             * code to hook in after the base device constructor, but before the device is fully initialized.
             *
             * It will typically be called by the DeviceServer.
             * The call is blocking and afterwards communication should happen only via slot calls.
             *
             * @param connection The broker connection for the device.
             * @param consumeBroadcasts If true, listen directly to broadcast messages (addressed to '*'), as usually
             * expected. Whoever sets this to false has to ensure that broadcast messages reach the Device in some other
             * way, otherwise the device will not work correctly.
             * @param timeServerId The id of the time server to be used by the device - usually set by the DeviceServer.
             */
            virtual void finalizeInternalInitialization(const karabo::net::Broker::Pointer& connection,
                                                        bool consumeBroadcasts, const std::string& timeServerId) = 0;

            // public since called by DeviceServer
            /**
             * A slot called by the device server if the external time ticks update to synchronize
             * this device with the timing system.
             *
             * @param id: current train id
             * @param sec: current system seconds
             * @param frac: current fractional seconds
             * @param period: interval between subsequent ids in microseconds
             */
            virtual void slotTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                      unsigned long long period) = 0;

            // public since called by DeviceServer
            /**
             * If the device receives time-server updates via slotTimeTick, this hook will be called for every id
             * in sequential order. The time stamps (sec + frac) of subsequent ids might be identical - though they
             * are usually spaced by period.
             * Can be overwritten in derived classes.
             *
             * @param id: train id
             * @param sec: unix seconds
             * @param frac: fractional seconds (i.e. attoseconds)
             * @param period: interval between ids in microseconds
             */
            virtual void onTimeUpdate(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                      unsigned long long period) {}

           protected:
            // protected since called in Device<FSM>::slotTimeTick
            /**
             * A hook which is called if the device receives external time-server update, i.e. if slotTimeTick on the
             * device server is called.
             * Can be overwritten by derived classes.
             *
             * @param id: train id
             * @param sec: unix seconds
             * @param frac: fractional seconds (i.e. attoseconds)
             * @param period: interval between ids in microseconds
             */
            virtual void onTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                    unsigned long long period) {}
        };

        /**
         * @class Device
         * @brief all Karabo devices derive from this class
         *
         * The Device class is the base class for all Karabo devices.
         * It provides for a standardized set of basic properties that
         * all devices share and provides interaction with its configuration
         * and properties.
         *
         * Devices are defined by their expected parameters; a list of properties
         * and commands that are known to the distributed system at static time.
         * These parameters describe a devices Schema, which in turn describes
         * the possible configurations of the device.
         *
         * Devices may come in two flavors: one's with a full-fledged, strict
         * finite state machine (FSM), embedded into the device through templated
         * inheritance, and those with a simple FSM, the noFSM, where states are
         * updated within the device logic. The Device class defaults to the latter
         * if no template parameter is given.
         */
        template <class FSM = NoFsm>
        class Device : public BaseDevice, public FSM {
            /// Validators to validate...
            karabo::util::Validator m_validatorIntern; /// ...internal updates via 'Device::set'
            karabo::util::Validator m_validatorExtern; /// ...external updates via 'Device::slotReconfigure'

            std::shared_ptr<DeviceClient> m_deviceClient;

            std::string m_classId;
            std::string m_serverId;
            std::string m_deviceId;
            // To be injected at initialization time; for internal use only.
            std::string m_timeServerId;

            unsigned long long m_timeId;
            unsigned long long m_timeSec;    // seconds
            unsigned long long m_timeFrac;   // attoseconds
            unsigned long long m_timePeriod; // microseconds
            mutable std::mutex m_timeChangeMutex;

            mutable std::mutex m_objectStateChangeMutex;
            karabo::util::Hash m_parameters;
            karabo::util::Schema m_staticSchema;
            karabo::util::Schema m_injectedSchema;
            karabo::util::Schema m_fullSchema;
            std::map<std::string, karabo::util::Schema> m_stateDependentSchema;

            karabo::util::Epochstamp m_lastBrokerErrorStamp;


           public:
            // Derived classes shall use "<packageName>-<repositoryVersion>" as their version
            KARABO_CLASSINFO(Device, "Device", karabo::util::Version::getVersion())

            /**
             * The expected parameter section of the Device class, known at
             * static time. The basic parameters described here are available
             * for all devices, many of them being expert or admin visible only.
             *
             * @param expected: a Schema to which these parameters will be
             *                  appended.
             */
            static void expectedParameters(karabo::util::Schema& expected) {
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
                      .description(
                            "The device instance ID uniquely identifies a device instance in the distributed system")
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

                INT32_ELEMENT(expected)
                      .key("visibility")
                      .displayedName("Visibility")
                      .description("Configures who is allowed to see this device at all")
                      .assignmentOptional()
                      .defaultValue(karabo::util::Schema::OBSERVER)
                      .adminAccess()
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
                      .description(
                            "Average time interval between remote message sending and processing it in this device.")
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

                FSM::expectedParameters(expected);
            }

            /**
             * Construct a device with a given configuration. The configuration
             * Hash may contain any of the following entries:
             *
             * _serverId_: a string representing the server this device is
             *             running on. If not given the device assumes to run
             *             in stand-alone mode.
             *
             * _deviceId_: a string representing this device's id, part of the
             *             unique identifier in the distributed system. If not
             *             given it defaults to _none_.
             *
             * @param configuration
             */
            Device(const karabo::util::Hash& configuration) : m_lastBrokerErrorStamp(0ull, 0ull) {
                // Set serverId
                if (configuration.has("_serverId_")) configuration.get("_serverId_", m_serverId);
                else m_serverId = KARABO_NO_SERVER;

                // Set instanceId
                if (configuration.has("_deviceId_")) configuration.get("_deviceId_", m_deviceId);
                else m_deviceId = "__none__";

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

            /**
             * The destructor will reset the DeviceClient attached to this device.
             */
            virtual ~Device() {
                KARABO_LOG_FRAMEWORK_TRACE
                      << "Device::~Device() dtor : m_deviceClient.use_count()=" << m_deviceClient.use_count() << "\n"
                      << karabo::util::StackTrace();
                m_deviceClient.reset();
            };

            /**
             * This function allows to communicate to other (remote) devices.
             * Any device contains also a controller for other devices (DeviceClient)
             * which is returned by this function.
             *
             * @return DeviceClient instance
             */
            DeviceClient& remote() {
                if (!m_deviceClient) {
                    // Initialize an embedded device client (for composition)
                    m_deviceClient = std::make_shared<DeviceClient>(shared_from_this(), false);
                    m_deviceClient->initialize();
                }
                return *(m_deviceClient);
            }

            /**
             * Updates the state/properties of the device. This function automatically notifies any observers in the
             * distributed system.
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @param value The corresponding value (of corresponding data-type)
             */
            template <class ValueType>
            void set(const std::string& key, const ValueType& value) {
                this->set<ValueType>(key, value, getActualTimestamp());
            }

            void set(const std::string& key, const karabo::util::State& state) {
                set(key, state, getActualTimestamp());
            }

            void set(const std::string& key, const karabo::util::AlarmCondition& condition) {
                set(key, condition, getActualTimestamp());
            }

            enum class VectorUpdate {

                removeOne = -2, // only first occurrence
                removeAll = -1, // all occurrences
                add = 1,
                addIfNotIn = 2
            };

            /**
             * Concurrency safe update of vector property
             *
             * Does not work for Hash (i.e. table element) due to Hash equality just checking similarity.
             * Removing might be unreliable for VECTOR_FLOAT or VECTOR_DOUBLE due to floating point equality issues.
             *
             * @param key of the vector property to update
             * @param updates: items to remove from property vector (starting at the front) or to add (at the end)
             * @param updateType: indicates update type - applied individually to all items in 'updates'
             * @param timestamp: timestamp to assign to updated vector property (e.g. getTimestamp())
             */
            template <class ItemType>
            void setVectorUpdate(const std::string& key, const std::vector<ItemType>& updates, VectorUpdate updateType,
                                 const karabo::util::Timestamp& timestamp) {
                // Get current value and update as requested
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                std::vector<ItemType> vec(m_parameters.get<std::vector<ItemType>>(key));
                switch (updateType) {
                    case VectorUpdate::add:
                        vec.insert(vec.end(), updates.begin(), updates.end());
                        break;
                    case VectorUpdate::addIfNotIn:
                        for (const ItemType& update : updates) {
                            auto it = std::find(vec.begin(), vec.end(), update);
                            if (it == vec.end()) vec.push_back(update);
                        }
                        break;
                    case VectorUpdate::removeOne:
                        for (auto itUpdate = updates.begin(); itUpdate != updates.end(); ++itUpdate) {
                            auto itInVec = std::find(vec.begin(), vec.end(), *itUpdate);
                            if (itInVec != vec.end()) {
                                vec.erase(itInVec);
                            }
                        }
                        break;
                    case VectorUpdate::removeAll:
                        auto itNewEnd = std::remove_if(vec.begin(), vec.end(), [&updates](const ItemType& item) {
                            return (std::find(updates.begin(), updates.end(), item) != updates.end());
                        });
                        vec.resize(itNewEnd - vec.begin()); // remove_if does not yet shrink
                        break;
                }

                // Now publish the new value
                karabo::util::Hash h;
                std::vector<ItemType>& vecInHash = h.bindReference<std::vector<ItemType>>(key);
                vecInHash.swap(vec);
                setNoLock(h, timestamp);
            }

            /**
             * Updates the state of the device. This function automatically notifies any observers in the distributed
             * system.
             *
             * Any updates are validated against the device schema and rejected if they are not
             * appropriate for the current device state or are of wrong type. During validation
             * alarm bounds are evaluated and alarms on properties will be raised if alarm
             * conditions are met. Additionally, the distributed system is notified of these
             * alarms.
             *
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @param value The corresponding value (of corresponding data-type)
             * @param timestamp The time of the value change
             */
            template <class ValueType>
            void set(const std::string& key, const ValueType& value, const karabo::util::Timestamp& timestamp) {
                karabo::util::Hash h(key, value);
                this->set(h, timestamp);
            }

            void set(const std::string& key, const karabo::util::State& state,
                     const karabo::util::Timestamp& timestamp) {
                karabo::util::Hash h(key, state.name());
                h.setAttribute(key, KARABO_INDICATE_STATE_SET, true);
                set(h, timestamp);
            }

            void set(const std::string& key, const karabo::util::AlarmCondition& condition,
                     const karabo::util::Timestamp& timestamp) {
                karabo::util::Hash h(key, condition.asString());
                h.setAttribute(key, KARABO_INDICATE_ALARM_SET, true);
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                setNoLock(h, timestamp);
                // also set the fields attribute
                m_parameters.setAttribute(key, KARABO_ALARM_ATTR, condition.asString());
            }

            /**
             * Writes a hash to the specified channel. The hash internally must
             * follow exactly the data schema as defined in the expected parameters.
             *
             * If 'data' contains an 'NDArray', consider to use the overload of this method that has the 'safeNDArray'
             * flag. If that flag can be set to 'true', data copies can be avoided:
             *
             * writeChannel(channelName, data, getActualTimestamp(), true);
             *
             * @param channelName The output channel name
             * @param data Hash with the data
             *
             * Thread safety:
             * The 'writeChannel(..)' methods and 'signalEndOfStream(..)' must not be called concurrently
             * for the same 'channelName'.
             */
            void writeChannel(const std::string& channelName, const karabo::util::Hash& data) {
                this->writeChannel(channelName, data, this->getActualTimestamp());
            }

            /**
             * Writes a hash to the specified channel. The hash internally must
             * follow exactly the data schema as defined in the expected parameters.
             *
             * @param channelName The output channel name
             * @param data Hash with the data
             * @param timestamp A user provided timestamp (if e.g. retrieved from h/w)
             * @param safeNDArray Boolean that should be set to 'true' if 'data' contains any 'NDArray' and their data
             *                    is not changed after this 'writeChannel'. Otherwise, data will be copied if needed,
             *                    i.e. when the output channel has to queue or serves inner-process receivers.
             *
             * Thread safety:
             * The 'writeChannel(..)' methods and 'signalEndOfStream(..)' must not be called concurrently
             * for the same 'channelName'.
             */
            void writeChannel(const std::string& channelName, const karabo::util::Hash& data,
                              const karabo::util::Timestamp& timestamp, bool safeNDArray = false) {
                using namespace karabo::xms;
                OutputChannel::Pointer channel = this->getOutputChannel(channelName);
                // Provide proper meta data information, as well as correct train- and timestamp
                OutputChannel::MetaData meta(m_instanceId + ":" + channelName, timestamp);
                channel->write(data, meta);
                channel->update(safeNDArray);
            }

            /**
             * Signals an end-of-stream event (EOS) on the output channel identified
             * by channelName
             * @param channelName: the name of the output channel.
             *
             * Thread safety:
             * The 'writeChannel(..)' methods and 'signalEndOfStream(..)' must not be called concurrently
             * for the same 'channelName'.
             */
            void signalEndOfStream(const std::string& channelName) {
                this->getOutputChannel(channelName)->signalEndOfStream();
            }

            /**
             * Updates the state/properties of the device with all key/value pairs given in the hash.
             *
             * Any updates are validated against the device schema and rejected if they are not
             * appropriate for the current device state or are of wrong type. During validation
             * alarm bounds are evaluated and alarms on properties will be raised if alarm
             * conditions are met. Additionally, the distributed system is notified of these
             * alarms.
             * For those paths in 'hash' which do not already have time stamp attributes assigned as tested by
             * Timestamp::hashAttributesContainTimeInformation(hash.getAttributes(<path>))),
             * the actual timestamp is chosen.
             *
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param hash Hash of updated internal parameters
             *             (must be in current full schema, e.g. since declared in the expectedParameters function)
             */
            void set(const karabo::util::Hash& hash) {
                this->set(hash, getActualTimestamp());
            }

            /**
             * Updates the state of the device with all key/value pairs given in the hash
             *
             * Any updates are validated against the device schema and rejected if they are not
             * appropriate for the current device state or are of wrong type. During validation
             * alarm bounds are evaluated and alarms on properties will be raised if alarm
             * conditions are met. Additionally, the distributed system is notified of these
             * alarms.
             *
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param hash Hash of updated internal parameters
             *             (must be in current full schema, e.g. since declared in the expectedParameters function)
             * @param timestamp to indicate when the set occurred - but is ignored for paths in 'hash'
             *                  that already have time stamp attributes as tested by
             *                  Timestamp::hashAttributesContainTimeInformation(hash.getAttributes(<path>)))
             */
            void set(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp) {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                setNoLock(hash, timestamp);
            }

           private:
            /**
             * Internal method for set(Hash, Timestamp), requiring m_objectStateChangeMutex to be locked
             */
            void setNoLock(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp) {
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

           public:
            /**
             * Updates the state of the device with all key/value pairs given in the hash.
             * In contrast to the set function, no validation is performed.
             *
             * @param key identifying the property to update
             * @param value: updated value
             */
            template <class ValueType>
            void setNoValidate(const std::string& key, const ValueType& value) {
                this->setNoValidate<ValueType>(key, value, getActualTimestamp());
            }

            /**
             * Updates the state of the device with all key/value pairs given in the hash.
             * In contrast to the set function, no validation is performed.
             *
             * @param key identifying the property to update
             * @param value: updated value
             * @param timestamp optional timestamp to indicate when the set occurred.
             */
            template <class ValueType>
            void setNoValidate(const std::string& key, const ValueType& value,
                               const karabo::util::Timestamp& timestamp) {
                karabo::util::Hash h(key, value);
                this->setNoValidate(h, timestamp);
            }

            /**
             * Updates the state of the device with all key/value pairs given in the hash.
             * In contrast to the set function, no validation is performed.
             *
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
             */
            void setNoValidate(const karabo::util::Hash& hash) {
                this->setNoValidate(hash, getActualTimestamp());
            }
            /**
             * Updates the state of the device with all key/value pairs given in the hash.
             * In contrast to the set function, no validation is performed.
             *
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
             * @param timestamp optional timestamp to indicate when the set occurred.
             */
            void setNoValidate(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp) {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                this->setNoValidateNoLock(hash, timestamp);
            }

           private:
            /**
             * Internal version of setNoValidate(hash, timestamp) that requires m_objectStateChangeMutex to be locked
             */
            void setNoValidateNoLock(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp) {
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

           public:
            /**
             * Retrieves the current value of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            T get(const std::string& key) const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);

                try {
                    const karabo::util::Hash::Attributes& attrs =
                          m_fullSchema.getParameterHash().getNode(key).getAttributes();
                    if (attrs.has(KARABO_SCHEMA_LEAF_TYPE)) {
                        const int leafType = attrs.get<int>(KARABO_SCHEMA_LEAF_TYPE);
                        if (leafType == karabo::util::Schema::STATE) {
                            if (typeid(T) == typeid(karabo::util::State)) {
                                return *reinterpret_cast<const T*>(
                                      &karabo::util::State::fromString(m_parameters.get<std::string>(key)));
                            }
                            throw KARABO_PARAMETER_EXCEPTION("State element at " + key +
                                                             " may only return state objects");
                        }
                        if (leafType == karabo::util::Schema::ALARM_CONDITION) {
                            if (typeid(T) == typeid(karabo::util::AlarmCondition)) {
                                return *reinterpret_cast<const T*>(
                                      &karabo::util::AlarmCondition::fromString(m_parameters.get<std::string>(key)));
                            }
                            throw KARABO_PARAMETER_EXCEPTION("Alarm condition element at " + key +
                                                             " may only return alarm condition objects");
                        }
                    }

                    return m_parameters.get<T>(key);
                } catch (const karabo::util::Exception&) {
                    KARABO_RETHROW_AS(
                          KARABO_PARAMETER_EXCEPTION(getInstanceId() + " failed to retrieve parameter '" + key + "'"));
                }
                return *static_cast<T*>(NULL); // never reached. Keep it to make the compiler happy.
            }

            /**
             * Retrieves the current value of any device parameter (that was defined in the expectedParameters function)
             * The value is casted on the fly into the desired type.
             * NOTE: This function is considerably slower than the simple get() functionality
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            T getAs(const std::string& key) const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                try {
                    return m_parameters.getAs<T>(key);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(
                          KARABO_PARAMETER_EXCEPTION("Error whilst retrieving parameter (" + key + ") from device"));
                }
            }

            /**
             * Retrieves all expected parameters of this device
             * @return Schema object containing all expected parameters
             */
            karabo::util::Schema getFullSchema() const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                return m_fullSchema;
            }

            /**
             * Append a schema to the existing device schema
             * @param schema to be appended -  may also contain existing elements to overwrite their
             *                attributes like min/max values/sizes, alarm ranges, etc.
             *                If it contains Input-/OutputChannels, they are (re-)created.
             *                If previously an InputChannel existed under the same key, its data/input/endOfStream
             * handlers are kept for the recreated InputChannel.
             * @param unused parameter, kept for backward compatibility.
             */
            void appendSchema(const karabo::util::Schema& schema, const bool /*unused*/ = false) {
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
                        KARABO_LOG_FRAMEWORK_DEBUG << "appendSchema triggers creation of output channel '"
                                                   << outToCreate << "'";
                        prepareOutputChannel(outToCreate);
                    }
                }

                KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Schema appended";
            }

            /**
             * Replace existing schema descriptions by static (hard coded in expectedParameters) part and
             * add additional (dynamic) descriptions. Previous additions will be removed.
             *
             * @param schema additional, dynamic schema - may also contain existing elements to overwrite their
             *                attributes like min/max values/sizes, alarm ranges, etc.
             *                If it contains Input-/OutputChannels, they are (re-)created (and previously added ones
             * removed). If previously an InputChannel existed under the same key, its data/input/endOfStream handlers
             *                are kept for the recreated InputChannel.
             * @param unused parameter, kept for backward compatibility.
             */
            void updateSchema(const karabo::util::Schema& schema, const bool /*unused*/ = false) {
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
                        KARABO_LOG_FRAMEWORK_DEBUG << "updateSchema triggers creation of output channel '"
                                                   << outToCreate << "'";
                        prepareOutputChannel(outToCreate);
                    }
                }

                KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Schema updated";
            }

            /**
             * Converts a device parameter key into its aliased key (must be defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return Aliased representation of the parameter
             */
            template <class AliasType>
            AliasType getAliasFromKey(const std::string& key) const {
                try {
                    std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                    return m_fullSchema.getAliasFromKey<AliasType>(key);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(
                          KARABO_PARAMETER_EXCEPTION("Error whilst retrieving alias from parameter (" + key + ")"));
                    return AliasType(); // compiler happiness line - requires that AliasType is default constructable...
                }
            }

            /**
             * Converts a device parameter alias into the original key (must be defined in the expectedParameters
             * function)
             * @param key A valid parameter-alias of the device (must be defined in the expectedParameters function)
             * @return The original name of the parameter
             */
            template <class AliasType>
            std::string getKeyFromAlias(const AliasType& alias) const {
                try {
                    std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                    return m_fullSchema.getKeyFromAlias(alias);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst retrieving parameter from alias (" +
                                                                 karabo::util::toString(alias) + ")"));
                    return std::string(); // compiler happiness line...
                }
            }

            /**
             * Checks if the argument is a valid alias of some key, i.e. defined in the expectedParameters function
             * @param alias Arbitrary argument of arbitrary type
             * @return true if it is an alias found in one of three containers of parameters:
             * "reconfigurable", "initial" or "monitored",  otherwise false
             */
            template <class T>
            const bool aliasHasKey(const T& alias) const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                return m_fullSchema.aliasHasKey(alias);
            }

            /**
             * Checks if some alias is defined for the given key
             * @param key in expectedParameters mapping
             * @return true if the alias exists
             */
            bool keyHasAlias(const std::string& key) const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                return m_fullSchema.keyHasAlias(key);
            }

            /**
             * Checks the type of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return The enumerated internal reference type of the value
             */
            karabo::util::Types::ReferenceType getValueType(const std::string& key) const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                return m_fullSchema.getValueType(key);
            }

            /**
             * Retrieves the current configuration.
             * If no argument is given, all parameters (those described in the expected parameters section) are
             * returned. A subset of parameters can be retrieved by specifying one or more tags.
             * @param tags The tags (separated by comma) the parameter must carry to be retrieved
             * @return A Hash containing the current value of the selected configuration
             */
            karabo::util::Hash getCurrentConfiguration(const std::string& tags = "") const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                if (tags.empty()) return m_parameters;
                karabo::util::Hash filtered;
                karabo::util::HashFilter::byTag(m_fullSchema, m_parameters, filtered, tags);
                return filtered;
            }

            /**
             * Retrieves a slice of the current configuration.
             *
             * @param paths of the configuration which should be returned (as declared in expectedParameters)
             *              (method throws ParameterExcepton if a non-existing path is given)
             * @return Hash with the current values and attributes (e.g. timestamp) of the selected configuration
             *
             */
            karabo::util::Hash getCurrentConfigurationSlice(const std::vector<std::string>& paths) const {
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

            /**
             * Return a tag filtered version of the input Hash. Tags are as defined
             * in the device schema
             * @param hash to filter
             * @param tags to filter by
             * @return a filtered version of the input Hash.
             */
            karabo::util::Hash filterByTags(const karabo::util::Hash& hash, const std::string& tags) const {
                karabo::util::Hash filtered;
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                karabo::util::HashFilter::byTag(m_fullSchema, hash, filtered, tags);
                return filtered;
            }

            /**
             * Return the serverId of the server this device is running on
             * @return
             */
            const std::string& getServerId() const {
                return m_serverId;
            }

            /**
             * Return a State object holding the current unified state of
             * the device.
             * @return
             */
            const karabo::util::State getState() {
                return this->get<karabo::util::State>("state");
            }

            /**
             * Update the state of the device, using "actual timestamp".
             *
             * Will also update the instanceInfo describing this device instance (if new or old State are ERROR).
             *
             * @param currentState: the state to update to
             */
            // Note that for inheritance from BaseFsm (in contrast to NoFsm), this overrides a virtual function!
            void updateState(const karabo::util::State& currentState) {
                updateState(currentState, karabo::util::Hash(), getActualTimestamp());
            }

            /**
             * Update the state of the device, using "actual timestamp".
             *
             * Will also update the instanceInfo describing this device instance (if new or old State are ERROR).
             *
             * @param currentState: the state to update to
             * @param other: a Hash to set other properties in the same state update message,
             *               time stamp attributes to its paths have precedence over the actual timestamp
             */
            void updateState(const karabo::util::State& currentState, const karabo::util::Hash& other) {
                updateState(currentState, other, getActualTimestamp());
            }

            /**
             * Update the state of the device, using given timestamp.
             *
             * Will also update the instanceInfo describing this device instance (if new or old State are ERROR).
             *
             * @param currentState: the state to update to
             * @param timestamp: time stamp to assign to the state property and the properties in 'other'
             *                   (if the latter do not have specified timestamp attributes)
             *
             */
            void updateState(const karabo::util::State& currentState, const karabo::util::Timestamp& timestamp) {
                updateState(currentState, karabo::util::Hash(), timestamp);
            }

            /**
             * Update the state of the device, using given timestamp.
             *
             * Will also update the instanceInfo describing this device instance (if new or old State are ERROR).
             *
             * @param currentState: the state to update to
             * @param other: a Hash to set other properties in the same state update message,
             *               time stamp attributes to its paths have precedence over the given 'timestamp'
             * @param timestamp: time stamp to assign to the state property and the properties in 'other'
             *                   (if the latter do not have specified timestamp attributes)
             *
             */
            void updateState(const karabo::util::State& currentState, karabo::util::Hash other,
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

            /**
             * You can override this function to handle default caught exceptions differently
             * @param shortMessage short error message
             * @param detailedMessage detailed error message
             */
            KARABO_DEPRECATED void exceptionFound(const std::string& shortMessage,
                                                  const std::string& detailedMessage) const {
                KARABO_LOG_ERROR << detailedMessage;
            }

            KARABO_DEPRECATED virtual void exceptionFound(const karabo::util::Exception& e) {
                KARABO_LOG_ERROR << e;
            }

            // void notify("ERROR", const std::string& shortMessage, const std::string& detailedMessage)

            // This function will polymorphically be called by the FSM template
            // TODO Make it private

            virtual void onNoStateTransition(const std::string& typeId, int state) {
                std::string eventName(typeId);
                boost::regex re(".*\\d+(.+Event).*");
                boost::smatch what;
                bool result = boost::regex_search(typeId, what, re);
                if (result && what.size() == 2) {
                    eventName = what.str(1);
                }
                KARABO_LOG_WARN << "Current state of device \"" << getInstanceId()
                                << "\" does not allow a transition for event \"" << eventName << "\".";
            }


            /**
             * Execute a command on this device
             * @param command
             */
            void execute(const std::string& command) const {
                call("", command);
            }

            /**
             * Execute a command with one argument on this device
             * @param command
             * @param a1
             */
            template <class A1>
            void execute(const std::string& command, const A1& a1) const {
                call("", command, a1);
            }

            /**
             * Execute a command with two arguments on this device
             * @param command
             * @param a1
             * @param a2
             */
            template <class A1, class A2>
            void execute(const std::string& command, const A1& a1, const A2& a2) const {
                call("", command, a1, a2);
            }

            /**
             * Execute a command with three arguments on this device
             * @param command
             * @param a1
             * @param a2
             * @param a3
             */
            template <class A1, class A2, class A3>
            void execute(const std::string& command, const A1& a1, const A2& a2, const A3& a3) const {
                call("", command, a1, a2, a3);
            }

            /**
             * Execute a command with four arguments on this device
             * @param command
             * @param a1
             * @param a2
             * @param a3
             * @param a4
             */
            template <class A1, class A2, class A3, class A4>
            void execute(const std::string& command, const A1& a1, const A2& a2, const A3& a3, const A4& a4) const {
                call("", command, a1, a2, a3, a4);
            }

            /**
             * Get the current alarm condition the device is in
             * @return
             */
            karabo::util::AlarmCondition getAlarmCondition() const {
                return this->get<karabo::util::AlarmCondition>("alarmCondition");
            }

            /**
             * Set the global alarm condition
             * @param condition to set
             * @param needsAcknowledging if this condition will require acknowledgment on the alarm service
             * @param description an optional description of the condition. Consider including remarks on how to resolve
             */
            void setAlarmCondition(const karabo::util::AlarmCondition& condition, bool needsAcknowledging = false,
                                   const std::string& description = std::string()) {
                using namespace karabo::util;

                const Timestamp timestamp(getActualTimestamp());
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                Hash h;
                h.set("alarmCondition", condition.asString()).setAttribute(KARABO_INDICATE_ALARM_SET, true);
                // also set the fields attribute to this condition
                this->setNoValidateNoLock(h, timestamp);
                this->m_parameters.setAttribute("alarmCondition", KARABO_ALARM_ATTR, condition.asString());
            }

            /**
             * Get the alarm condition for a specific property
             * @param key of the property to get the condition for
             * @param sep optional separator to use in the key path
             * @return the alarm condition of the property
             */
            const karabo::util::AlarmCondition& getAlarmCondition(const std::string& key,
                                                                  const std::string& sep = ".") const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                const std::string& propertyCondition =
                      this->m_parameters.template getAttribute<std::string>(key, KARABO_ALARM_ATTR, sep.at(0));
                return karabo::util::AlarmCondition::fromString(propertyCondition);
            }

            /**
             * Query if the property at path has rolling statistics enabled
             * @param path
             * @return
             */
            bool hasRollingStatistics(const std::string& path) const {
                return this->getFullSchema().hasRollingStatistics(path);
            }

            /**
             * Get a pointer to the rolling statistics for the property at path
             * @param path
             * @return a pointer to the rolling statistics object keeping track
             * of the statistics for the property identified by path.
             */
            karabo::util::RollingWindowStatistics::ConstPointer getRollingStatistics(const std::string& path) const {
                return m_validatorIntern.getRollingStatistics(path);
            }

            void slotTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac,
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

            /**
             * Append Schema to change/set maximum size information for path - if paths does not exist, throw exception
             *
             * This is similar to the more general appendSchema, but dedicated to a common use case.
             *
             * Caveat: This does not recreate an output channel if its schema is changed
             *
             * @param path  indicates the parameter which should be a Vector- or TableElement
             * @param value is the new maximum size of the parameter
             * @param emitFlag indicates if others should be informed about this Schema update.
             *                 If this method is called for a bunch of paths, it is recommended to
             *                 set this to true only for the last call.
             */
            void appendSchemaMaxSize(const std::string& path, unsigned int value, bool emitFlag = true) {
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

           protected: // Functions and Classes
            virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration) {}

            virtual void postReconfigure() {}

            virtual void preDestruction() {}

            virtual bool allowLock() const {
                return true;
            }

            /**
             * Returns the actual timestamp. The Trainstamp part of Timestamp is extrapolated from the last values
             * received via slotTimeTick (or zero if no time ticks received yet).
             * To receive time ticks, the server of the device has to be connected to a time server.
             *
             * @return the actual timestamp
             */
            inline karabo::util::Timestamp getActualTimestamp() const {
                return getTimestamp(karabo::util::Epochstamp()); // i.e. epochstamp for now
            }

            /**
             * Returns the Timestamp for given Epochstamp. The Trainstamp part of Timestamp is extrapolated forward or
             * backward from the last values received via slotTimeTick
             * (or zero if no time ticks received yet).
             * To receive time ticks, the server of the device has to be connected to a time server.
             * @param epoch for that the time stamp is searched for
             * @return the matching timestamp, consisting of epoch and the corresponding Trainstamp
             */
            karabo::util::Timestamp getTimestamp(const karabo::util::Epochstamp& epoch) const {
                unsigned long long id = 0;
                {
                    std::lock_guard<std::mutex> lock(m_timeChangeMutex);
                    if (m_timePeriod > 0) {
                        const karabo::util::Epochstamp epochLastReceived(m_timeSec, m_timeFrac);
                        // duration is always positive, irrespective whether epoch or epochLastReceived is more recent
                        const karabo::util::TimeDuration duration = epoch.elapsed(epochLastReceived);
                        const unsigned long long nPeriods = (duration.getTotalSeconds() * 1000000ull +
                                                             duration.getFractions(karabo::util::MICROSEC)) /
                                                            m_timePeriod;
                        if (epochLastReceived <= epoch) {
                            id = m_timeId + nPeriods;
                        } else if (m_timeId >= nPeriods + 1ull) { // sanity check
                            id = m_timeId - nPeriods - 1ull;
                        } else {
                            KARABO_LOG_FRAMEWORK_WARN << "Bad input: (train)Id zero since epoch = " << epoch.toIso8601()
                                                      << "; from time server: epoch = " << epochLastReceived.toIso8601()
                                                      << ", id = " << m_timeId << ", period = " << m_timePeriod
                                                      << " mus";
                        }
                    }
                }
                return karabo::util::Timestamp(epoch, karabo::util::Trainstamp(id));
            }

           private: // Functions
            /**
             * This function will typically be called by the DeviceServer.
             * The call is blocking and afterwards communication should happen only via slot calls.
             *
             * @param connection The broker connection for the device.
             * @param consumeBroadcasts If false, do not listen directly to broadcast messages (addressed to '*').
             *                          Whoever sets this to false has to ensure that broadcast messages reach the
             *                          Device in some other way.
             *  @param timeServerId The id of the time server to be used by the device - usually set by the
             *                      DeviceServer.
             */
            void finalizeInternalInitialization(const karabo::net::Broker::Pointer& connection, bool consumeBroadcasts,
                                                const std::string& timeServerId) {
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
                instanceInfo.set("visibility", this->get<int>("visibility"));
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

                // Initialize FSM slots (the interface of this function must be inherited from the templated FSM)
                this->initFsmSlots(); // requires template CONCEPT

                // Initialize Device slots
                this->initDeviceSlots();

                // Register guard for slot calls
                this->registerSlotCallGuardHandler(std::bind(&karabo::core::Device<FSM>::slotCallGuard, this, _1, _2));

                // Register updateLatencies handler -
                // bind_weak not needed since (and as long as...) handler will not be posted on event loop
                this->registerPerformanceStatisticsHandler(
                      std::bind(&karabo::core::Device<FSM>::updateLatencies, this, _1));

                // Register message consumption error handler - bind_weak not needed as above
                this->registerBrokerErrorHandler(std::bind(&karabo::core::Device<FSM>::onBrokerError, this, _1));

                // Instantiate all channels - needs mutex
                {
                    std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                    this->initChannels(m_fullSchema);
                }
                //
                // Then start SignalSlotable: communication (incl. system registration) starts and thus parallelism!
                //
                SignalSlotable::start();

                KARABO_LOG_FRAMEWORK_INFO << "'" << m_classId << "' (version '"
                                          << this->get<std::string>("classVersion") << "') with deviceId: '"
                                          << this->getInstanceId() << "' got started" << " on server '"
                                          << this->getServerId() << "'.";

                //
                // Finally do everything that requires full participation in the system
                //

                // Connect input channels - requires SignalSlotable to be started
                this->connectInputChannels(boost::system::error_code());

                // Start the state machine (call initialization methods in case of noFsm)
                // Do that on the event loop since any blocking should not influence the success of the instantiation
                // procedure, i.e. the device server slot that starts the device should reply immediately, irrespective
                // of what startFsm() does. We wrap the call to startFsm() to treat exceptions.
                net::EventLoop::getIOService().post(util::bind_weak(&Device<FSM>::wrapStartFsm, this));
            }

            void wrapStartFsm() {
                try {
                    this->startFsm(); // (This function must be inherited from the templated base class (it's a
                                      // concept!)
                } catch (const std::exception& e) {
                    const std::string exceptionTxt(e.what());
                    KARABO_LOG_ERROR << "The instance with deviceId " << this->getInstanceId()
                                     << " is going down due to an exception in initialization ..." << exceptionTxt;
                    // Indicate in the status and kill the device
                    set("status", std::string("Initialization failed: ") += exceptionTxt);
                    this->call("", "slotKillDevice");
                }
            }

            void initClassId() {
                m_classId = getClassInfo().getClassId();
            }

            void initSchema() {
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

            void initDeviceSlots() {
                using namespace std;

                KARABO_SIGNAL("signalChanged", karabo::util::Hash /*configuration*/, string /*deviceId*/);

                KARABO_SYSTEM_SIGNAL("signalStateChanged", karabo::util::Hash /*configuration*/, string /*deviceId*/);

                KARABO_SYSTEM_SIGNAL("signalSchemaUpdated", karabo::util::Schema /*deviceSchema*/, string /*deviceId*/);

                KARABO_SLOT(slotReconfigure, karabo::util::Hash /*reconfiguration*/)

                KARABO_SLOT(slotGetConfiguration)

                KARABO_SLOT(slotGetConfigurationSlice, karabo::util::Hash)

                KARABO_SLOT(slotGetSchema, bool /*onlyCurrentState*/);

                KARABO_SLOT(slotKillDevice)

                KARABO_SLOT(slotUpdateSchemaAttributes, std::vector<karabo::util::Hash>);

                KARABO_SLOT(slotClearLock);

                KARABO_SLOT(slotGetTime, karabo::util::Hash /* UNUSED */);

                KARABO_SLOT(slotGetSystemInfo, karabo::util::Hash /* UNUSED */);
            }

            /**
             *  Called to setup pipeline channels, will
             *  recursively go through the given schema, assuming it to be at least
             *  a part of the schema of the device.
             *  Needs to be called with m_objectStateChangeMutex being locked.
             *  *
             *  * @param schema: the schema to traverse
             *  * @param topLevel: std::string: empty or existing path of full
             *  *                  schema of the device
             *  */
            void initChannels(const karabo::util::Schema& schema, const std::string& topLevel = "") {
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

            /**
             * Create OutputChannel for given path and take care to set handlers needed
             * Needs to be called with m_objectStateChangeMutex being locked.
             * @param path
             */
            void prepareOutputChannel(const std::string& path) {
                KARABO_LOG_FRAMEWORK_INFO << "'" << this->getInstanceId() << "' creates output channel '" << path
                                          << "'";
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
                        channel->registerShowStatisticsHandler(
                              [weakThis, path](const std::vector<unsigned long long>& rb,
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

            /**
             * Create InputChannel for given path and take care to set handlers needed
             * Needs to be called with m_objectStateChangeMutex being locked.
             * @param path
             */
            void prepareInputChannel(const std::string& path) {
                KARABO_LOG_FRAMEWORK_INFO << "'" << this->getInstanceId() << "' creates input channel '" << path << "'";
                using karabo::xms::InputChannel;
                using namespace std::placeholders;
                InputChannel::Handlers handlers;
                // If there was already an InputChannel, "rescue" any registered handlers for new channel
                InputChannel::Pointer channel = getInputChannelNoThrow(path);
                if (channel) {
                    handlers = channel->getRegisteredHandlers();
                }
                channel = createInputChannel(
                      path, m_parameters, handlers.dataHandler, handlers.inputHandler, handlers.eosHandler,
                      util::bind_weak(&Device<FSM>::trackInputChannelConnections, this, path, _1, _2));
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

            void trackInputChannelConnections(const std::string& inputChannel, const std::string& outputChannel,
                                              karabo::net::ConnectionStatus status) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Input channel '" << inputChannel << "': connection status for '"
                                           << outputChannel << "' changed: " << static_cast<int>(status);
                if (status == karabo::net::ConnectionStatus::CONNECTING ||
                    status == karabo::net::ConnectionStatus::DISCONNECTING) {
                    // Ignore any intermediate connection status
                    return;
                }

                const VectorUpdate type =
                      (status == karabo::net::ConnectionStatus::DISCONNECTED ? VectorUpdate::addIfNotIn
                                                                             : VectorUpdate::removeOne);
                setVectorUpdate(inputChannel + ".missingConnections", std::vector<std::string>({outputChannel}), type,
                                getActualTimestamp());
            }

            /**
             * This function is called by SignalSlotable to verify if a slot may
             * be called from remote. The function only checks slots that are
             * mentioned in the expectedParameter section ("DeviceSlots")
             * @param slotName: name of the slot
             * @param callee: the calling remote, can be unknown
             *
             * The following checks are performed:
             *
             * 1) Is this device locked by another device? If the lockedBy field
             * is non-empty and does not match the callee's instance id, the call will
             * be rejected
             *
             * 2) Is the slot callable from the current state, i.e. is the
             * current state specified as an allowed state for the slot.
             */
            void slotCallGuard(const std::string& slotName, const std::string& callee) {
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

            void ensureSlotIsValidUnderCurrentLock(const std::string& slotName, const std::string& callee) {
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

            void ensureSlotIsValidUnderCurrentState(const std::string& slotName) {
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
                        msg << "Command \"" << slotName << "\" is not allowed in current state \""
                            << currentState.name() << "\" of device \"" << m_deviceId << "\".";
                        throw KARABO_LOGIC_EXCEPTION(msg.str());
                    }
                }
            }

            void slotGetConfiguration() {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                reply(m_parameters, m_deviceId);
            }

            void slotGetConfigurationSlice(const karabo::util::Hash& info) {
                const auto& paths = info.get<std::vector<std::string>>("paths");
                reply(getCurrentConfigurationSlice(paths));
            }

            void slotGetSchema(bool onlyCurrentState) {
                if (onlyCurrentState) {
                    const karabo::util::State& currentState = getState();
                    const karabo::util::Schema schema(getStateDependentSchema(currentState));
                    reply(schema, m_deviceId);
                } else {
                    std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                    reply(m_fullSchema, m_deviceId);
                }
            }

            void slotReconfigure(const karabo::util::Hash& newConfiguration) {
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

            std::pair<bool, std::string> validate(const karabo::util::Hash& unvalidated,
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

            void applyReconfiguration(const karabo::util::Hash& reconfiguration) {
                {
                    std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                    m_parameters.merge(reconfiguration);
                }
                KARABO_LOG_DEBUG << "After user interaction:\n" << reconfiguration;
                if (m_validatorExtern.hasReconfigurableParameter())
                    emit("signalStateChanged", reconfiguration, getInstanceId());
                else emit("signalChanged", reconfiguration, getInstanceId());
            }

            void slotKillDevice() {
                // It is important to know who gave us the kill signal
                std::string senderId = getSenderInfo("slotKillDevice")->getInstanceIdOfSender();
                this->preDestruction(); // Give devices a chance to react
                this->stopFsm();
                if (senderId == m_serverId) { // Our server killed us
                    KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << " is going down as instructed by server";
                } else { // Someone else wants to see us dead, we should inform our server
                    KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << " is going down as instructed by \"" << senderId
                                              << "\"";
                    call(m_serverId, "slotDeviceGone", m_deviceId);
                }
            }

            karabo::util::Schema getStateDependentSchema(const karabo::util::State& state) {
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

            void updateLatencies(const karabo::util::Hash::Pointer& performanceMeasures) {
                if (this->get<bool>("performanceStatistics.enable")) {
                    // Keys and values of 'performanceMeasures' are defined in
                    // SignalSlotable::updatePerformanceStatistics and expectedParameters has to foresee this content
                    // under node "performanceStatistics".
                    this->set(karabo::util::Hash("performanceStatistics", *performanceMeasures));
                }
            }

            void onBrokerError(const std::string& message) {
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

            /**
             * Updates attributes in the device's runtime schema.
             * @param updates: updated attributes, expected to be of form Hash("instanceId", str, "updates",
             * vector<Hash>) where each entry in updates is of the form Hash("path", str, "attribute", str, "value",
             * valueType)
             *
             * reply is of the form Hash("success" bool, "instanceId", str, "updatedSchema", Schema, "requestedUpdate",
             * vector<Hash>) where success indicates a successful update, instanceId the device that performed the
             * update updatedSchema the new valid schema, regardless of success or not, and requestedUpdates the
             * original update request, as received through onUpdateAttributes
             */
            void slotUpdateSchemaAttributes(const std::vector<karabo::util::Hash>& updates) {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                // Whenever updating the m_fullSchema, we have to clear the cache
                m_stateDependentSchema.clear();
                bool success = m_fullSchema.applyRuntimeUpdates(updates);
                if (success) {
                    m_injectedSchema.applyRuntimeUpdates(updates);
                    // Notify the distributed system
                    emit("signalSchemaUpdated", m_fullSchema, m_deviceId);
                }
                reply(karabo::util::Hash("success", success, "instanceId", getInstanceId(), "updatedSchema",
                                         m_fullSchema, "requestedUpdate", updates));
            }

            /**
             * Clear any lock on this device
             */
            void slotClearLock() {
                set("lockedBy", std::string());
            }

            /**
             * Internal method to retrieve time information of this device.
             */
            karabo::util::Hash getTimeInfo() {
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
            /**
             * Returns the actual time information of this device.
             *
             * @param info an unused (at least for now) Hash parameter that
             *        has been added to fit the generic slot call
             *        (Hash in, Hash out) of the GUI server/client protocol.
             *
             * This slot reply is a Hash with the following keys:
             *   - ``time`` and its attributes provide an actual timestamp
             *     with train Id information.
             *   - ``timeServerId`` the id of the time server configured for
             *     the DeviceServer of this device; "None" when there's no time
             *     server configured.
             *   - ``reference`` and its attributes provide the latest
             *     timestamp information received from the timeserver.
             */
            void slotGetTime(const karabo::util::Hash& /* unused */) {
                const karabo::util::Hash result(this->getTimeInfo());
                reply(result);
            }
            /**
             * Returns the actual system information of this device.
             *
             * @param info an unused (at least for now) Hash parameter that
             *        has been added to fit the generic slot call
             *        (Hash in, Hash out) of the GUI server/client protocol.
             *
             * This slot reply is a Hash with the following keys:
             *   - ``user`` the actual user running this device
             *   - ``broker`` the current connected broker node
             *
             * and all keys provided by ``slotGetTime``.
             */
            void slotGetSystemInfo(const karabo::util::Hash& /* unused */) {
                using namespace karabo::util;
                Hash result("timeInfo", this->getTimeInfo());
                result.set("broker", m_connection->getBrokerUrl());
                auto user = getlogin(); // Little caveat: getlogin() is a Linux function
                result.set("user", (user ? user : "none"));
                reply(result);
            }
        };

    } // namespace core
} // namespace karabo

#endif
