/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_DEVICE_HH
#define	KARABO_CORE_DEVICE_HH

#include "NoFsm.hh"
#include "DeviceClient.hh"
#include "Lock.hh"

#include "karabo/util/MetaTools.hh"
#include "karabo/util/State.hh"
#include "karabo/util.hpp"
#include "karabo/util/StackTrace.hh"
#include "karabo/util/OverwriteElement.hh"
#include "karabo/util/RollingWindowStatistics.hh"
#include "karabo/net/utils.hh"
#include "karabo/xms/SlotElement.hh"
#include "karabo/xms.hpp"
#include "karabo/log/Logger.hh"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <unordered_set>
#include <unistd.h>

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace core {

        // Convenient logging
#define KARABO_LOG_DEBUG this->log() << krb_log4cpp::Priority::DEBUG
#define KARABO_LOG_INFO  this->log() << krb_log4cpp::Priority::INFO
#define KARABO_LOG_WARN  this->log() << krb_log4cpp::Priority::WARN
#define KARABO_LOG_ERROR this->log() << krb_log4cpp::Priority::ERROR

#define KARABO_NO_SERVER "__none__"

        enum Capabilities {

            PROVIDES_SCENES = (1u << 0),
            PROVIDES_MACROS = (1u << 1),
            // add future capabilities as bitmask:
            // SOME_OTHER_FUTURE_CAPABILITY = (1u << 2),
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

            virtual ~BaseDevice() {
            }

            /**
             * This method is called to finalize initialization of a device. It is needed to allow user 
             * code to hook in after the base device constructor, but before the device is fully initialized.
             *
             * It will typically be called by the DeviceServer.
             * The call is blocking and afterwards communication should happen only via slot calls.
             *
             * @param consumeBroadcasts If true, listen directly to broadcast messages (addressed to '*'), as usually expected.
             *                          Whoever sets this to false has to enure that broadcast messages reach the Device
             *                          in some other way, otherwise the device will not work correctly.
             */
            virtual void finalizeInternalInitialization(bool consumeBroadcasts) = 0;

            /*
             * A pure virtual method to be overwritten to return a tag-filtered
             * configuration of derived classes
             *
             * @param tags: tags to filter the return configuration by
             * @return: a Hash containing the (filtered) documentation
             */
            virtual karabo::util::Hash getCurrentConfiguration(const std::string& tags = "") const = 0;

            /**
             * A slot called by the device server if the external or possibly generated internally time ticks have to be
             * passed to synchronize this device with the timing system.
             *
             * @param id: current train id
             * @param sec: current system seconds
             * @param frac: current fractional seconds
             * @param period: interval between subsequent ids in microseconds
             */
            virtual void slotTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac, unsigned long long period) = 0;

            /**
             * A hook which is called if the device receives external time-server update, i.e. if slotTimeTick on the 
             * device server is called.
             * Can be overwritten by derived classes.
             *
             * @param id: train id
             * @param sec: unix seconds
             * @param frac: fractional seconds (i.e. attoseconds)
             * @param period: interval between ids im microseconds
             */
            virtual void onTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac, unsigned long long period) = 0;

            /**
             * Check if the device is configured to use the time server ticks
             * @return boolean
             */
            virtual bool useTimeServer() const = 0;
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

            karabo::util::Validator m_validatorIntern;
            karabo::util::Validator m_validatorExtern;

            boost::shared_ptr<DeviceClient> m_deviceClient;

            std::string m_classId;
            std::string m_serverId;
            std::string m_deviceId;

            std::map<std::string, karabo::util::Schema> m_stateDependentSchema;
            mutable boost::mutex m_stateDependentSchemaMutex;

            // Regular expression for error detection in state word
            boost::regex m_errorRegex;

            // progressBar related
            int m_progressMin;
            int m_progressMax;

            unsigned long long m_timeId;
            unsigned long long m_timeSec; // seconds
            unsigned long long m_timeFrac; // attoseconds
            unsigned long long m_timePeriod; // microseconds
            mutable boost::mutex m_timeChangeMutex;
            unsigned long long m_lastTimeIdUpdated; // used only in onTimeUpdateHelper
            mutable boost::mutex m_lastTimeIdUpdatedMutex;

            krb_log4cpp::Category* m_log;

            mutable boost::mutex m_objectStateChangeMutex;
            karabo::util::Hash m_parameters;
            karabo::util::Schema m_staticSchema;
            karabo::util::Schema m_injectedSchema;
            karabo::util::Schema m_fullSchema;

            karabo::util::AlarmCondition m_globalAlarmCondition;
            std::set<std::string> m_accumulatedGlobalAlarms;

            karabo::util::Epochstamp m_lastBrokerErrorStamp;


        public:

            KARABO_CLASSINFO(Device, "Device", "1.0")

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

                STRING_ELEMENT(expected).key("_serverId_")
                        .displayedName("_ServerID_")
                        .description("Do not set this property, it will be set by the device-server")
                        .adminAccess()
                        .assignmentOptional().noDefaultValue()
                        .init()
                        .commit();

                STRING_ELEMENT(expected).key("_deviceId_")
                        .displayedName("_DeviceID_")
                        .description("Do not set this property, it will be set by the device-server")
                        .adminAccess()
                        .assignmentOptional().noDefaultValue()
                        .init()
                        .commit();

                NODE_ELEMENT(expected).key("_connection_")
                        .displayedName("_Connection_")
                        .description("Do not set this property, it will be set by the device-server")
                        .appendParametersOf<karabo::net::JmsConnection>()
                        .expertAccess()
                        .commit();

                INT32_ELEMENT(expected).key("visibility")
                        .displayedName("Visibility")
                        .description("Configures who is allowed to see this device at all")
                        .assignmentOptional().defaultValue(karabo::util::Schema::OBSERVER)
                        .adminAccess()
                        .reconfigurable()
                        .commit();

                STRING_ELEMENT(expected).key("deviceId")
                        .displayedName("DeviceID")
                        .description("The device instance ID uniquely identifies a device instance in the distributed system")
                        .readOnly()
                        .commit();

                STRING_ELEMENT(expected).key("classId")
                        .displayedName("ClassID")
                        .description("The (factory)-name of the class of this device")
                        .expertAccess()
                        .readOnly()
                        .initialValue(Device::classInfo().getClassId())
                        .commit();

                STRING_ELEMENT(expected).key("classVersion")
                        .displayedName("Class version")
                        .description("The version of the class of this device defined in KARABO_CLASSINFO")
                        .expertAccess()
                        .readOnly()
                        .initialValue(Device::classInfo().getVersion())
                        .commit();

                STRING_ELEMENT(expected).key("serverId")
                        .displayedName("ServerID")
                        .description("The device-server on which this device is running on")
                        .expertAccess()
                        .readOnly()
                        .commit();

                STRING_ELEMENT(expected).key("hostName")
                        .displayedName("Host")
                        .description("The name of the host where this device runs")
                        .expertAccess()
                        .readOnly()
                        .commit();

                INT32_ELEMENT(expected).key("pid")
                        .displayedName("Process ID")
                        .description("The unix process ID of the device (i.e. of the server")
                        .expertAccess().readOnly().initialValue(0)
                        .commit();

                STATE_ELEMENT(expected).key("state")
                        .displayedName("State")
                        .description("The current state the device is in")
                        .initialValue(State::UNKNOWN)
                        .commit();

                STRING_ELEMENT(expected).key("status")
                        .displayedName("Status")
                        .description("A more detailed status description")
                        .readOnly().initialValue("")
                        .commit();

                ALARM_ELEMENT(expected).key("alarmCondition")
                        .displayedName("Alarm condition")
                        .description("The current alarm condition of the device. "
                                     "Evaluates to the highest condition on any"
                                     " property if not set manually.")
                        .initialValue(AlarmCondition::NONE)
                        .commit();

                STRING_ELEMENT(expected).key("lockedBy")
                        .displayedName("Locked by")
                        .reconfigurable()
                        .assignmentOptional().defaultValue("")
                        .expertAccess()
                        .commit();

                SLOT_ELEMENT(expected).key("slotClearLock")
                        .displayedName("Clear Lock")
                        .expertAccess()
                        .commit();


                STRING_ELEMENT(expected).key("lastCommand")
                        .displayedName("Last command")
                        .description("The last slot called.")
                        .adminAccess()
                        .readOnly().initialValue("")
                        .commit();

                BOOL_ELEMENT(expected).key("archive")
                        .displayedName("Archive")
                        .description("Decides whether the properties of this device will be logged or not")
                        .reconfigurable()
                        .expertAccess()
                        .assignmentOptional().defaultValue(true)
                        .commit();

                BOOL_ELEMENT(expected).key("useTimeserver")
                        .displayedName("Use Timeserver")
                        .description("Decides whether to use time and train ID from TimeServer device")
                        .init()
                        .expertAccess()
                        .assignmentOptional().defaultValue(false)
                        .commit();

                INT32_ELEMENT(expected).key("progress")
                        .displayedName("Progress")
                        .description("The progress of the current action")
                        .readOnly()
                        .initialValue(0)
                        .commit();

                INT32_ELEMENT(expected).key("heartbeatInterval")
                        .displayedName("Heartbeat interval")
                        .description("The heartbeat interval")
                        .assignmentOptional().defaultValue(120)
                        .minInc(10) // avoid too much traffic - 10 is minimum of server as well
                        .adminAccess()
                        .commit();

                NODE_ELEMENT(expected).key("performanceStatistics")
                        .displayedName("Performance Statistics")
                        .description("Accumulates some statistics")
                        .expertAccess()
                        .commit();

                BOOL_ELEMENT(expected).key("performanceStatistics.messagingProblems")
                        .displayedName("Messaging problems")
                        .description("If true, there is a problem consuming broker messages")
                        .expertAccess()
                        // threshold is exclusive: value true fulfills "> false" and triggers alarm whereas false does not
                        // .alarmHigh(false)
                        // .info("Unreliable broker message consumption - consider restarting device!")
                        // .needsAcknowledging(true)
                        .readOnly().initialValue(false)
                        .commit();

                BOOL_ELEMENT(expected).key("performanceStatistics.enable")
                        .displayedName("Enable Performance Indicators")
                        .description("Enables some statistics to follow the performance of an individual device")
                        .reconfigurable()
                        .expertAccess()
                        .assignmentOptional().defaultValue(false)
                        .commit();

                FLOAT_ELEMENT(expected).key("performanceStatistics.processingLatency")
                        .displayedName("Processing latency")
                        .description("Average time interval between remote message sending and processing it in this device.")
                        .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                        .expertAccess()
                        .readOnly().initialValue(0.f)
                        .warnHigh(3000.f) // 3 s
                        .info("Long average time between message being sent and start of its processing")
                        .needsAcknowledging(false)
                        .alarmHigh(10000.f) // 10 s
                        .info("Very long average time between message being sent and start of its processing")
                        .needsAcknowledging(false)
                        .commit();

                UINT32_ELEMENT(expected).key("performanceStatistics.maxProcessingLatency")
                        .displayedName("Maximum latency")
                        .description("Maximum processing latency within averaging interval.")
                        .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                        .expertAccess()
                        .readOnly().initialValue(0)
                        .commit();

                UINT32_ELEMENT(expected).key("performanceStatistics.numMessages")
                        .displayedName("Number of messages")
                        .description("Number of messages received within averaging interval.")
                        .unit(Unit::COUNT)
                        .expertAccess()
                        .readOnly().initialValue(0)
                        .commit();

                UINT32_ELEMENT(expected).key("performanceStatistics.maxEventLoopLatency")
                        .displayedName("Max. event loop latency")
                        .description("Maximum time interval between posting a message on the central event loop "
                                     "and processing it within averaging interval.")
                        .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                        .expertAccess()
                        .readOnly().initialValue(0)
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
            Device(const karabo::util::Hash& configuration)
                : m_errorRegex(".*error.*", boost::regex::icase)
                , m_lastTimeIdUpdated(0ull)
                , m_globalAlarmCondition(karabo::util::AlarmCondition::NONE)
                , m_lastBrokerErrorStamp(0ull, 0ull) {


                m_connection = karabo::util::Configurator<karabo::net::JmsConnection>::createNode("_connection_", configuration);

                // Make the configuration the initial state of the device
                m_parameters = configuration;

                // This is a hack until a better solution is found
                // Will remove a potential JmsConnection::Pointer instance from the m_parameters
                m_parameters.set("_connection_", karabo::util::Hash());
                m_parameters.set("hostName", net::bareHostName());

                m_timeId = 0;
                m_timeSec = 0;
                m_timeFrac = 0;
                m_timePeriod = 0; // zero as identifier of initial value used in slotTimeTick

                // Set serverId
                if (configuration.has("_serverId_")) configuration.get("_serverId_", m_serverId);
                else m_serverId = KARABO_NO_SERVER;

                // Set instanceId
                if (configuration.has("_deviceId_")) configuration.get("_deviceId_", m_deviceId);
                else m_deviceId = "__none__";

                // Setup the validation classes
                karabo::util::Validator::ValidationRules rules;
                rules.allowAdditionalKeys = false;
                rules.allowMissingKeys = true;
                rules.allowUnrootedConfiguration = true;
                rules.injectDefaults = false;
                rules.injectTimestamps = true;
                m_validatorIntern.setValidationRules(rules);
                m_validatorExtern.setValidationRules(rules);

                // Setup device logger
                m_log = &(karabo::log::Logger::getCategory(m_deviceId)); // TODO use later: "device." + instanceId
            }

            /**
             * The destructor will reset the DeviceClient attached to this device.
             */
            virtual ~Device() {
                KARABO_LOG_FRAMEWORK_TRACE << "Device::~Device() dtor : m_deviceClient.use_count()="
                        << m_deviceClient.use_count() << "\n"
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
                    m_deviceClient = boost::shared_ptr<DeviceClient > (new DeviceClient(shared_from_this()));
                }
                return *(m_deviceClient);
            }

            /**
             * Updates the state/properties of the device. This function automatically notifies any observers in the distributed system.
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

            /**
             * Updates the state of the device. This function automatically notifies any observers in the distributed system.
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

            void set(const std::string& key, const karabo::util::State& state, const karabo::util::Timestamp& timestamp) {
                karabo::util::Hash h(key, state.name());
                h.setAttribute(key, KARABO_INDICATE_STATE_SET, true);
                set(h, timestamp);
            }

            void set(const std::string& key, const karabo::util::AlarmCondition& condition, const karabo::util::Timestamp& timestamp) {
                karabo::util::Hash h(key, condition.asString());
                h.setAttribute(key, KARABO_INDICATE_ALARM_SET, true);
                set(h, timestamp);
                //also set the fields attribute
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                m_parameters.setAttribute(key, KARABO_ALARM_ATTR, condition.asString());
            }

            /**
             * Writes a hash to the specified channel. The hash internally must
             * follow exactly the data schema as defined in the expected parameters.
             * @param channelName The output channel name
             * @param data Hash with the data
             */
            void writeChannel(const std::string& channelName, const karabo::util::Hash& data) {
                this->writeChannel(channelName, data, this->getActualTimestamp());
            }

            /**
             * Writes a hash to the specified channel. The hash internally must
             * follow exactly the data schema as defined in the expected parameters.
             * @param channelName The output channel name
             * @param data Hash with the data
             * @param timestamp A user provided timestamp (if e.g. retrieved from h/w)
             */
            void writeChannel(const std::string& channelName, const karabo::util::Hash& data,
                              const karabo::util::Timestamp& timestamp) {
                using namespace karabo::xms;
                OutputChannel::Pointer channel = this->getOutputChannel(channelName);
                // Provide proper meta data information, as well as correct train- and timestamp
                OutputChannel::MetaData meta(m_instanceId + ":" + channelName, timestamp);
                channel->write(data, meta);
                channel->update();
            }

            /**
             * Signals an end-of-stream event (EOS) on the output channel identified
             * by channelName
             * @param channelName: the name of the output channel.
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
             *
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
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
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
             * @param timestamp optional timestamp to indicate when the set occurred
             */
            void set(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp) {
                using namespace karabo::util;

                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                karabo::util::Hash validated;
                std::pair<bool, std::string> result;

                const Hash previousParametersInAlarm(m_validatorIntern.getParametersInWarnOrAlarm());

                result = m_validatorIntern.validate(m_fullSchema, hash, validated, timestamp);

                if (result.first == false) {
                    KARABO_LOG_WARN << "Bad parameter setting attempted, validation reports: " << result.second;
                }

                // Check for parameters being in a bad condition
                const std::pair<bool, const AlarmCondition> resultingCondition =
                        evaluateAndUpdateAlarmCondition(!previousParametersInAlarm.empty(), previousParametersInAlarm, false);
                if (resultingCondition.first && resultingCondition.second.asString() != m_parameters.get<std::string>("alarmCondition")) {
                    Hash::Node& node = validated.set("alarmCondition", resultingCondition.second.asString());
                    Hash::Attributes& attributes = node.getAttributes();
                    timestamp.toHashAttributes(attributes);
                }

                //notify of alarm changes
                Hash changedAlarms;
                evaluateAlarmUpdates(previousParametersInAlarm, changedAlarms);
                if (!changedAlarms.get<Hash>("toClear").empty() || !changedAlarms.get<Hash>("toAdd").empty()) {
                    emit("signalAlarmUpdate", getInstanceId(), changedAlarms);
                }


                if (!validated.empty()) {
                    m_parameters.merge(validated, karabo::util::Hash::REPLACE_ATTRIBUTES);

                    // if Hash contains at least one reconfigurable parameter -> signalStateChanged
                    if (validated.has("state")) {
                        emit("signalStateChanged", validated, getInstanceId()); // more reliable delivery: timeToLive == 600000 (10min)
                        return;
                    }

                    // if Hash contains at least one reconfigurable parameter -> signalStateChanged
                    if (m_validatorIntern.hasReconfigurableParameter()) {
                        emit("signalStateChanged", validated, getInstanceId()); // more reliable delivery: timeToLive == 600000 (10min)
                        return;
                    }

                    // ... otherwise -> signalChanged
                    emit("signalChanged", validated, getInstanceId()); // less reliable delivery: timeToLive == 3000 (3 secs)
                }
            }

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
            void setNoValidate(const std::string& key, const ValueType& value, const karabo::util::Timestamp& timestamp) {
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
                using namespace karabo::util;
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);

                // TODO Think about attaching timestamps only to top level nodes (must check all other depending code)
                if (!hash.empty()) {
                    Hash tmp(hash);
                    std::vector<std::string> paths;
                    tmp.getPaths(paths);

                    BOOST_FOREACH(std::string path, paths) {
                        timestamp.toHashAttributes(tmp.getAttributes(path));
                    }
                    m_parameters.merge(tmp, Hash::REPLACE_ATTRIBUTES);

                    // if Hash contains 'state' key -> signalStateChanged
                    if (tmp.has("state")) {
                        emit("signalStateChanged", tmp, getInstanceId()); // more reliable delivery: timeToLive == 600000 (10min)
                        return;
                    }


                    // if Hash contains at least one reconfigurable parameter -> signalStateChanged

                    BOOST_FOREACH(std::string path, paths) {
                        if (m_fullSchema.has(path) && m_fullSchema.isAccessReconfigurable(path)) {
                            emit("signalStateChanged", tmp, getInstanceId()); // more reliable delivery: timeToLive == 600000 (10min)
                            return;
                        }
                    }

                    // ... otherwise -> signalChanged
                    emit("signalChanged", tmp, getInstanceId());
                }
            }

            /**
             * Retrieves the current value of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            T get(const std::string& key) const {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);

                try {
                    const karabo::util::Hash::Attributes& attrs = m_fullSchema.getParameterHash().getNode(key).getAttributes();
                    if (attrs.has(KARABO_SCHEMA_LEAF_TYPE)) {
                        const int leafType = attrs.get<int>(KARABO_SCHEMA_LEAF_TYPE);
                        if (leafType == karabo::util::Schema::STATE) {
                            if (typeid (T) == typeid (karabo::util::State)) {
                                return *reinterpret_cast<const T*> (&karabo::util::State::fromString(m_parameters.get<std::string>(key)));
                            }
                            throw KARABO_PARAMETER_EXCEPTION("State element at " + key + " may only return state objects");
                        }
                        if (leafType == karabo::util::Schema::ALARM_CONDITION) {
                            if (typeid (T) == typeid (karabo::util::AlarmCondition)) {
                                return *reinterpret_cast<const T*> (&karabo::util::AlarmCondition::fromString(m_parameters.get<std::string>(key)));
                            }
                            throw KARABO_PARAMETER_EXCEPTION("Alarm condition element at " + key + " may only return alarm condition objects");
                        }
                    }

                    return m_parameters.get<T>(key);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst retrieving parameter (" + key + ") from device:" +
                                                                 e.userFriendlyMsg()));
                }
                return *static_cast<T*> (NULL); // never reached. Keep it to make the compiler happy.
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
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                try {
                    return m_parameters.getAs<T>(key);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst retrieving parameter (" + key + ") from device"));
                }
            }

            /**
             * Use this function for any logging information.
             * @code
             * log() << Priority::DEBUG << "My logging message on debug priority";
             * log() << Priority::INFO << "My logging message on info priority";
             * log() << Priority::WARN << "My logging message on warn priority";
             * log() << Priority::ERROR << "My logging message on error priority";
             * @endcode
             * @return Logging object
             */
            krb_log4cpp::Category& log() const {
                return (*m_log);
            }

            /**
             * Retrieves all expected parameters of this device
             * @return Schema object containing all expected parameters
             */
            karabo::util::Schema getFullSchema() const {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                return m_fullSchema;
            }

            /**
             * Append a schema to the existing device schema
             * @param schema to be appended
             * @param keepParameters: if true, do not reset the configuration of
             * the device, for those parameters that validate against the new
             * schema
             */
            void appendSchema(const karabo::util::Schema& schema, const bool keepParameters = false) {
                KARABO_LOG_DEBUG << "Append Schema requested";
                karabo::util::Hash validated;
                karabo::util::Validator::ValidationRules rules;
                rules.allowAdditionalKeys = true;
                rules.allowMissingKeys = true;
                rules.allowUnrootedConfiguration = true;
                rules.injectDefaults = true;
                rules.injectTimestamps = true;
                karabo::util::Validator v(rules);
                v.validate(schema, karabo::util::Hash(), validated, getActualTimestamp());
                {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);

                    // Clear cache
                    m_stateDependentSchema.clear();

                    // Save injected
                    m_injectedSchema.merge(schema);

                    // Merge to full schema
                    m_fullSchema.merge(m_injectedSchema);

                    // Notify the distributed system
                    emit("signalSchemaUpdated", m_fullSchema, m_deviceId);

                    // Merge all parameters
                    if (keepParameters) validated.merge(m_parameters);
                }

                set(validated);

                KARABO_LOG_INFO << "Schema updated";
            }

            /**
             * Replace existing schema descriptions by static (hard coded in expectedParameters) part and
             * add additional (dynamic) descriptions
             * @param schema replacing the existing schema.
             * @param keepParameters: if true, do not reset the configuration of
             * the device, for those parameters that validate against the new
             * schema
             */
            void updateSchema(const karabo::util::Schema& schema, const bool keepParameters = false) {

                KARABO_LOG_DEBUG << "Update Schema requested";
                karabo::util::Hash validated;
                karabo::util::Validator::ValidationRules rules;
                rules.allowAdditionalKeys = true;
                rules.allowMissingKeys = true;
                rules.allowUnrootedConfiguration = true;
                rules.injectDefaults = true;
                rules.injectTimestamps = true;
                karabo::util::Validator v(rules);
                v.validate(schema, karabo::util::Hash(), validated, getActualTimestamp());
                {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    // Clear previously injected parameters
                    std::vector<std::string> keys = m_injectedSchema.getKeys();

                    BOOST_FOREACH(std::string key, keys) {
                        if (m_parameters.has(key)) m_parameters.erase(key);
                    }

                    // Clear cache
                    m_stateDependentSchema.clear();

                    // Reset fullSchema
                    m_fullSchema = m_staticSchema;

                    // Save injected
                    m_injectedSchema = schema;

                    // Merge to full schema
                    m_fullSchema.merge(m_injectedSchema);

                    // Notify the distributed system
                    emit("signalSchemaUpdated", m_fullSchema, m_deviceId);

                    // Merge all parameters
                    if (keepParameters) validated.merge(m_parameters);
                }

                set(validated);

                KARABO_LOG_INFO << "Schema updated";
            }

            /**
             * Set the progress of an operation
             * @param value
             */
            void setProgress(const int value, const std::string& associatedText = "") {
                int v = (m_progressMin + value / float(m_progressMax - m_progressMin)) * 100;
                set("progress", v);
            }

            /**
             * Reset progress to the lowest value
             */
            void resetProgress() {
                set("progress", m_progressMin);
            }

            /**
             * Set the range for progress values
             * @param minimum
             * @param maximum
             */
            void setProgressRange(const int minimum, const int maximum) {
                m_progressMin = minimum;
                m_progressMax = maximum;
            }

            /**
             * Converts a device parameter key into its aliased key (must be defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return Aliased representation of the parameter
             */
            template <class AliasType>
            AliasType getAliasFromKey(const std::string& key) const {
                try {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    return m_fullSchema.getAliasFromKey<AliasType>(key);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst retrieving alias from parameter (" + key + ")"));
                }
            }

            /**
             * Converts a device parameter alias into the original key (must be defined in the expectedParameters function)
             * @param key A valid parameter-alias of the device (must be defined in the expectedParameters function)
             * @return The original name of the parameter
             */
            template <class AliasType>
            std::string getKeyFromAlias(const AliasType& alias) const {
                try {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    return m_fullSchema.getKeyFromAlias(alias);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst retrieving parameter from alias (" + karabo::util::toString(alias) + ")"));
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
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                return m_fullSchema.aliasHasKey(alias);
            }

            /**
             * Checks if some alias is defined for the given key
             * @param key in expectedParameters mapping
             * @return true if the alias exists
             */
            bool keyHasAlias(const std::string& key) const {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                return m_fullSchema.keyHasAlias(key);
            }

            /**
             * Checks the type of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return The enumerated internal reference type of the value
             */
            karabo::util::Types::ReferenceType getValueType(const std::string& key) const {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                return m_fullSchema.getValueType(key);
            }

            /**
             * Retrieves the current configuration.
             * If no argument is given, all parameters (those described in the expected parameters section) are returned.
             * A subset of parameters can be retrieved by specifying one or more tags.
             * @param tags The tags the parameter must carry to be retrieved
             * @return A Hash containing the current value of the selected configuration
             */
            karabo::util::Hash getCurrentConfiguration(const std::string& tags = "") const {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                if (tags.empty()) return m_parameters;
                karabo::util::Hash filtered;
                karabo::util::HashFilter::byTag(m_fullSchema, m_parameters, filtered, tags);
                return filtered;
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
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
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
             * Update the state of the device. Will also update the instanceInfo
             * describing this device instance
             * @param cs: the state to update to
             */
            void updateState(const karabo::util::State& cs) {
                try {
                    const std::string& currentState = cs.name();
                    KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << ".updateState: \"" << currentState << "\".";
                    if (getState().name() != currentState) {
                        set("state", cs);
                        if (boost::regex_match(currentState, m_errorRegex)) {
                            updateInstanceInfo(karabo::util::Hash("status", "error"));
                        } else {
                            // Reset the error status - protect against non-initialised instanceInfo
                            const karabo::util::Hash info(getInstanceInfo());
                            if (!info.has("status") || info.get<std::string>("status") == "error") {
                                updateInstanceInfo(karabo::util::Hash("status", "ok"));
                            }
                        }
                    }
                    // Reply new state to interested event initiators
                    reply(currentState);

                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW
                }
            }

            /**
             * You can override this function to handle default caught exceptions differently
             * @param shortMessage short error message
             * @param detailedMessage detailed error message
             */
            KARABO_DEPRECATED void exceptionFound(const std::string& shortMessage, const std::string& detailedMessage) const {
                KARABO_LOG_ERROR << detailedMessage;
            }

            KARABO_DEPRECATED virtual void exceptionFound(const karabo::util::Exception& e) {
                KARABO_LOG_ERROR << e;
            }

            //void notify("ERROR", const std::string& shortMessage, const std::string& detailedMessage)

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
             * If the device receives time-server updates via slotTimeTick, this hook will be called for every id
             * in sequential order. The time stamps (sec + frac) of subsequent ids might be identical - though they
             * are usually spaced by period.
             * Can be overwritten in derived classes.
             *
             * @param id: train id
             * @param sec: unix seconds
             * @param frac: fractional seconds (i.e. attoseconds)
             * @param period: interval between ids microseconds
             */
            virtual void onTimeUpdate(unsigned long long id, unsigned long long sec, unsigned long long frac, unsigned long long period) {
            }

            bool useTimeServer() const {
                return this->get<bool>("useTimeserver");
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
            const karabo::util::AlarmCondition & getAlarmCondition() const {
                return karabo::util::AlarmCondition::fromString(this->get<std::string>("alarmCondition"));
            }

            /**
             * Set the global alarm condition
             * @param condition to set
             * @param needsAcknowledging if this condition will require acknowledgment on the alarm service
             * @param description an optional description of the condition. Consider including remarks on how to resolve
             */
            void setAlarmCondition(const karabo::util::AlarmCondition & condition, bool needsAcknowledging = true, const std::string& description = std::string()) {

                using namespace karabo::util;

                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                // copy on purpose for previousGlobal
                const std::string previousGlobal = m_globalAlarmCondition.asString();
                m_globalAlarmCondition = condition;
                m_accumulatedGlobalAlarms.insert(previousGlobal);

                std::pair<bool, const AlarmCondition> result = this->evaluateAndUpdateAlarmCondition(true, Hash(), true);
                if (result.first && result.second.asString() != m_parameters.get<std::string>("alarmCondition")) {
                    lock.unlock();
                    Hash h;
                    h.set("alarmCondition", result.second.asString()).setAttribute(KARABO_INDICATE_ALARM_SET, true);
                    //also set the fields attribute to this condition
                    this->setNoValidate(h);
                    this->m_parameters.setAttribute("alarmCondition", KARABO_ALARM_ATTR, result.second.asString());
                }
                // emit signal to alarm service
                Hash emitHash;
                Hash& toClear = emitHash.bindReference<Hash>("toClear");
                Hash& toAdd = emitHash.bindReference<Hash>("toAdd");
                const std::string& conditionString = condition.asString();
                if (condition.asString() == AlarmCondition::NONE.asString() && previousGlobal != AlarmCondition::NONE.asString()) {
                    const std::vector<std::string> alarmsToClear(m_accumulatedGlobalAlarms.begin(), m_accumulatedGlobalAlarms.end());
                    m_accumulatedGlobalAlarms.clear();
                    toClear.set("global", alarmsToClear);
                } else {
                    Hash::Node& propertyNode = toAdd.set("global", Hash());
                    Hash::Node& entryNode = propertyNode.getValue<Hash>().set(conditionString, Hash());
                    Hash& entry = entryNode.getValue<Hash>();

                    entry.set("type", conditionString);
                    entry.set("description", description);
                    entry.set("needsAcknowledging", needsAcknowledging);
                    Timestamp().toHashAttributes(entryNode.getAttributes()); // attach current time stamp
                }
                if (!emitHash.get<Hash>("toClear").empty() || !emitHash.get<Hash>("toAdd").empty()) {
                    emit("signalAlarmUpdate", getInstanceId(), emitHash);
                }
            }

            /**
             * Get the alarm condition for a specific property
             * @param key of the property to get the condition for
             * @param sep optional separator to use in the key path
             * @return the alarm condition of the property
             */
            const karabo::util::AlarmCondition & getAlarmCondition(const std::string & key, const std::string & sep = ".") const {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                const std::string& propertyCondition = this->m_parameters.template getAttribute<std::string>(key, KARABO_ALARM_ATTR, sep.at(0));
                return karabo::util::AlarmCondition::fromString(propertyCondition);
            }

            /**
             * Query if the property at path has rolling statistics enabled
             * @param path
             * @return
             */
            bool hasRollingStatistics(const std::string & path) const {
                return this->getFullSchema().hasRollingStatistics(path);
            }

            /**
             * Get a pointer to the rolling statistics for the property at path
             * @param path
             * @return a pointer to the rolling statistics object keeping track
             * of the statistics for the property identified by path.
             */
            karabo::util::RollingWindowStatistics::ConstPointer getRollingStatistics(const std::string & path) const {
                return m_validatorIntern.getRollingStatistics(path);
            }

            /**
             * Returns a hash containing the info field information
             * for current alarms on the device
             *
             * @return a hash with structure key: path to property -> alarm info (string)
             */
            const karabo::util::Hash getAlarmInfo() const {
                using namespace karabo::util;
                Hash info;
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                const Hash& h = m_validatorIntern.getParametersInWarnOrAlarm();
                for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
                    const Hash& desc = it->getValue<Hash>();
                    const AlarmCondition& cond = AlarmCondition::fromString(desc.get<std::string>("type"));
                    std::string propertyDotSep(it->getKey());
                    boost::replace_all(propertyDotSep, Validator::kAlarmParamPathSeparator, ".");
                    info.set(propertyDotSep, m_fullSchema.getInfoForAlarm(propertyDotSep, cond));
                }
                return info;

            }

            void slotTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac, unsigned long long period) {
                {
                    boost::mutex::scoped_lock lock(m_timeChangeMutex);
                    m_timeId = id;
                    m_timeSec = sec;
                    m_timeFrac = frac;
                    m_timePeriod = period;
                }
                // Since directly called from DeviceServer for all devices in sequence, we post the helper since it
                // might be blocking.
                karabo::net::EventLoop::getIOService().post(karabo::util::bind_weak(&Device<FSM>::onTimeUpdateHelper, this,
                                                                                    id, sec, frac, period));
            }

            void onTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac, unsigned long long period) {
            }

            /**
             * Append Schema to change/set maximum size information for path - if paths does not exist, throw exception
             *
             * This is similar to the more general appendSchema, but dedicated to a common use case.
             *
             * @param path  indicates the parameter which should be a Vector- or TableElement
             * @param value is the new maximum size of the parameter
             * @param emitFlag indicates if others should be informed about this Schema update.
             *                 If this method is called for a bunch of paths, it is recommended to
             *                 set this to true only for the last call.
             */
            void appendSchemaMaxSize(const std::string& path, unsigned int value, bool emitFlag = true) {
                using karabo::util::OVERWRITE_ELEMENT;
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
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

            virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration) {
            }

            virtual void postReconfigure() {
            }

            virtual void preDestruction() {
            }

            virtual bool allowLock() const {
                return true;
            }

            /**
             * Returns the actual timestamp. The Trainstamp part of Timestamp is extrapolated from the last values
             * received via slotTimeTick (or zero if no time ticks received yet, e.g. if useTimeserver is false).
             *
             * @return the actual timestamp
             */
            inline karabo::util::Timestamp getActualTimestamp() const {
                return getTimestamp(karabo::util::Epochstamp()); // i.e. epochstamp for now
            }

            /**
             * Returns the Timestamp for given Epochstamp. The Trainstamp part of Timestamp is extrapolated forward or
             * backward from the last values received via slotTimeTick
             * (or zero if no time ticks received yet, e.g. if useTimeserver is false).
             *
             * @param epoch for that the time stamp is searched for
             * @return the matching timestamp, consisting of epoch and the corresponding Trainstamp
             */
            karabo::util::Timestamp getTimestamp(const karabo::util::Epochstamp& epoch) const {
                unsigned long long id = 0;
                {
                    boost::mutex::scoped_lock lock(m_timeChangeMutex);
                    if (m_timePeriod > 0) {
                        const karabo::util::Epochstamp epochLastReceived(m_timeSec, m_timeFrac);
                        // duration is always positive, irrespective whether epoch or epochLastReceived is more recent
                        const karabo::util::TimeDuration duration = epoch.elapsed(epochLastReceived);
                        const unsigned long long nPeriods = (duration.getTotalSeconds() * 1000000ull + duration.getFractions(karabo::util::MICROSEC)) / m_timePeriod;
                        if (epochLastReceived <= epoch) {
                            id = m_timeId + nPeriods;
                        } else if (m_timeId >= nPeriods + 1ull) { // sanity check
                            id = m_timeId - nPeriods - 1ull;
                        } else {
                          KARABO_LOG_FRAMEWORK_WARN << "Bad input: (train)Id zero since "
                              << "epoch = " << epoch.toIso8601()
                              << "; from time server: epoch = " << epochLastReceived.toIso8601()
                              << ", id = " << m_timeId << ", period = " << m_timePeriod << " mus";
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
             * @param consumeBroadcasts If false, do not listen directly to broadcast messages (addressed to '*').
             *                          Whoever sets this to true has to enure that broadcast messages reach the Device
             *                          in some other way.
             */
            void finalizeInternalInitialization(bool consumeBroadcasts) {

                using namespace karabo::util;
                using namespace karabo::net;

                //
                // First set all parameters (which could not yet be set in constructor) and init the SignalSlotable
                //

                // These initializations or done here and not in the constructor
                // as they involve virtual function calls
                this->initClassId();
                this->initSchema();

                bool hasAvailableScenes = false;
                bool hasAvailableMacros = false;
                int  heartbeatInterval = 0;

                {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    // ClassId
                    m_parameters.set("classId", m_classId);
                    // DeviceId
                    m_parameters.set("deviceId", m_deviceId);
                    // ServerId
                    m_parameters.set("serverId", m_serverId);

                    // The following lines of code are needed to initially inject timestamps to the parameters
                    karabo::util::Hash validated;
                    std::pair<bool, std::string> result = m_validatorIntern.validate(m_fullSchema, m_parameters, validated, getActualTimestamp());
                    if (result.first == false) KARABO_LOG_WARN << "Bad parameter setting attempted, validation reports: " << result.second;
                    m_parameters.merge(validated, karabo::util::Hash::REPLACE_ATTRIBUTES);

                    // Do this under mutex protection
                    hasAvailableScenes = m_parameters.has("availableScenes");
                    hasAvailableMacros = m_parameters.has("availableMacros");
                    heartbeatInterval  = m_parameters.get<int>("heartbeatInterval");
                }

                // Prepare some info further describing this particular instance
                karabo::util::Hash instanceInfo;
                instanceInfo.set("type", "device");
                instanceInfo.set("classId", getClassInfo().getClassId());
                instanceInfo.set("serverId", m_serverId);
                instanceInfo.set("visibility", this->get<int >("visibility"));
                instanceInfo.set("compatibility", classInfo().getVersion());
                instanceInfo.set("host", net::bareHostName());
                instanceInfo.set("status", "ok");
                instanceInfo.set("archive", this->get<bool>("archive"));

                // the capabilities field specifies the optional capabilities a device provides.
                unsigned int capabilities = 0;
                if (hasAvailableScenes) capabilities |= Capabilities::PROVIDES_SCENES;
                if (hasAvailableMacros) capabilities |= Capabilities::PROVIDES_MACROS;
                instanceInfo.set("capabilities", capabilities);

                init(m_deviceId, m_connection, heartbeatInterval, instanceInfo, consumeBroadcasts);

                //
                // Now do all registrations etc. (Note that it is safe to register slots in the constructor)
                //

                // Initialize FSM slots (the interface of this function must be inherited from the templated FSM)
                this->initFsmSlots(); // requires template CONCEPT

                // Initialize Device slots
                this->initDeviceSlots();

                // Register guard for slot calls
                this->registerSlotCallGuardHandler(boost::bind(&karabo::core::Device<FSM>::slotCallGuard, this, _1, _2));

                // Register updateLatencies handler -
                // bind_weak not needed since (and as long as...) handler will not be posted on event loop
                this->registerPerformanceStatisticsHandler(boost::bind(&karabo::core::Device<FSM>::updateLatencies,
                                                                       this, _1));

                // Register message consumption error handler - bind_weak not needed as above
                this->registerBrokerErrorHandler(boost::bind(&karabo::core::Device<FSM>::onBrokerError, this, _1));

                // Instantiate all channels
                this->initChannels();

                //
                // Then start SignalSlotable: communication (incl. system registration) starts and thus parallelism!
                //
                SignalSlotable::start();

                KARABO_LOG_INFO << "'" << m_classId << "' with deviceId: '" << this->getInstanceId() << "' got started"
                        << " on server '" << this->getServerId() << "'.";

                //
                // Finally do everything that requires full participation in the system
                //

                // Connect input channels - requires SignalSlotable to be started
                this->connectInputChannels();

                // Start the state machine (call initialization methods in case of noFsm)
                this->startFsm(); // This function must be inherited from the templated base class (it's a concept!)

                this->set("pid", ::getpid());

                if (get<bool>("useTimeserver")) {
                    KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << " is configured to use the TimeServer";
                }
            }

            void initClassId() {
                m_classId = getClassInfo().getClassId();
            }

            void initSchema() {
                using namespace karabo::util;
                const Schema staticSchema = getSchema(m_classId, Schema::AssemblyRules(INIT | WRITE | READ));
                {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    // The static schema is the regular schema as assembled by parsing the expectedParameters functions
                    m_staticSchema = staticSchema; // Here we lack a Schema::swap(..)...
                    // At startup the static schema is identical with the runtime schema
                    m_fullSchema = m_staticSchema;
                }
            }

            void initDeviceSlots() {
                using namespace std;

                KARABO_SIGNAL2("signalChanged", karabo::util::Hash /*configuration*/, string /*deviceId*/);

                KARABO_SYSTEM_SIGNAL2("signalStateChanged", karabo::util::Hash /*configuration*/, string /*deviceId*/);

                KARABO_SYSTEM_SIGNAL2("signalSchemaUpdated", karabo::util::Schema /*deviceSchema*/, string /*deviceId*/);

                KARABO_SIGNAL2("signalAlarmUpdate", std::string, karabo::util::Hash);

                KARABO_SLOT(slotReconfigure, karabo::util::Hash /*reconfiguration*/)

                KARABO_SLOT(slotGetConfiguration)

                KARABO_SLOT(slotGetSchema, bool /*onlyCurrentState*/);

                KARABO_SLOT(slotKillDevice)

                KARABO_SLOT(slotReSubmitAlarms, karabo::util::Hash);

                KARABO_SLOT(slotUpdateSchemaAttributes, std::vector<karabo::util::Hash>);

                KARABO_SLOT(slotClearLock);
            }

            /**
             *  Called in beginning of run() to setup pipeline channels, will
             *  recursively go through the schema of the device
             *  *
             *  * @param topLevel: std::string: empty or existing path of full
             *  *                  schema of the device
             *  */
            void initChannels(const std::string& topLevel = "") {

                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                // Keys under topLevel, without leading "topLevel.":
                const std::vector<std::string>& subKeys = m_fullSchema.getKeys(topLevel);

                BOOST_FOREACH(const std::string &subKey, subKeys) {
                    // Assemble full path out of topLevel and subKey
                    const std::string key(topLevel.empty() ? subKey : (topLevel + '.') += subKey);
                    if (m_fullSchema.hasDisplayType(key)) {
                        const std::string& displayType = m_fullSchema.getDisplayType(key);
                        if (displayType == "OutputChannel") {
                            KARABO_LOG_FRAMEWORK_INFO << "'" << this->getInstanceId() << "' creates output channel '" << key << "'";
                            try {
                                createOutputChannel(key, m_parameters);
                            } catch (const karabo::util::NetworkException& e) {
                                KARABO_LOG_ERROR << e.userFriendlyMsg();
                            }
                        } else if (displayType == "InputChannel") {
                            KARABO_LOG_FRAMEWORK_INFO << "'" << this->getInstanceId() << "' creates input channel '" << key << "'";
                            createInputChannel(key, m_parameters);
                        } else {
                            KARABO_LOG_FRAMEWORK_DEBUG << "'" << this->getInstanceId() << "' does not create in-/output "
                                    << "channel for '" << key << "' since it's a '" << displayType << "'";
                        }
                    } else if (m_fullSchema.isNode(key)) {
                        // Recursive call going down the tree for channels within nodes
                        KARABO_LOG_FRAMEWORK_DEBUG << "'" << this->getInstanceId() << "' looks for input/output channels "
                                << "under node \"" << key << "\"";

                        lock.unlock(); // release lock before recursion
                        this->initChannels(key);
                        lock.lock();
                    }
                }
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
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    isSchemaSlot = m_fullSchema.has(slotName);
                }

                // Check whether the slot can be called given the current locking state
                if (allowLock() && (isSchemaSlot || slotName == "slotReconfigure") && slotName != "slotClearLock") {
                    ensureSlotIsValidUnderCurrentLock(slotName, callee);
                }

                // Check whether the slot can be called given the current device state
                if (isSchemaSlot) {
                    ensureSlotIsValidUnderCurrentState(slotName);
                }

                // Log the call of this slot by setting a parameter of the device
                if (isSchemaSlot) set("lastCommand", slotName);
            }

            void ensureSlotIsValidUnderCurrentLock(const std::string& slotName, const std::string& callee) {
                const std::string lockHolder = get<std::string>("lockedBy");
                if (!lockHolder.empty()) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "'" << getInstanceId() << "' is locked by " << lockHolder
                            << " and called by '" << callee << "'";
                    if (callee != "unknown" && callee != lockHolder) {
                        std::ostringstream msg;
                        msg << "Command " << "\"" << slotName << "\"" << " is not allowed as device is locked by "
                                << "\"" << lockHolder << ".";
                        throw KARABO_LOCK_EXCEPTION(msg.str());
                    }
                }
            }

            void ensureSlotIsValidUnderCurrentState(const std::string& slotName) {
                std::vector<karabo::util::State> allowedStates;
                {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    if (m_fullSchema.hasAllowedStates(slotName)) {
                        allowedStates = m_fullSchema.getAllowedStates(slotName);
                    }
                }
                if (!allowedStates.empty()) {
                    const karabo::util::State currentState = getState();
                    if (std::find(allowedStates.begin(), allowedStates.end(), currentState) == allowedStates.end()) {
                        std::ostringstream msg;
                        msg << "Command " << "\"" << slotName << "\"" << " is not allowed in current state "
                                << "\"" << currentState.name() << "\" of device " << "\"" << m_deviceId << "\".";
                        throw KARABO_LOGIC_EXCEPTION(msg.str());
                    }
                }
            }

            void slotGetConfiguration() {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                reply(m_parameters, m_deviceId);
            }

            void slotGetSchema(bool onlyCurrentState) {
                if (onlyCurrentState) {
                    const karabo::util::State& currentState = getState();
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    const karabo::util::Schema schema(getStateDependentSchema(currentState));
                    reply(schema, m_deviceId);
                } else {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    reply(m_fullSchema, m_deviceId);
                }
            }

            void slotReconfigure(const karabo::util::Hash& newConfiguration) {
                if (newConfiguration.empty()) return;

                karabo::util::Hash validated;
                std::pair<bool, std::string > result = validate(newConfiguration, validated);

                if (result.first == true) { // is a valid reconfiguration

                    // Give device-implementer a chance to specifically react on reconfiguration event by polymorphically calling back
                    preReconfigure(validated);

                    // nothing to do if empty after preReconfigure
                    if (!validated.empty()) {

                        // Merge reconfiguration with current state
                        applyReconfiguration(validated);

                    }
                    //post reconfigure action
                    this->postReconfigure();

                } else { // not a valid reconfiguration
                    throw KARABO_PARAMETER_EXCEPTION(result.second);
                }
            }

            std::pair<bool, std::string> validate(const karabo::util::Hash& unvalidated, karabo::util::Hash& validated) {
                // Retrieve the current state of the device instance
                const karabo::util::State& currentState = getState();
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                const karabo::util::Schema whiteList(getStateDependentSchema(currentState));
                KARABO_LOG_DEBUG << "Incoming (un-validated) reconfiguration:\n" << unvalidated;
                std::pair<bool, std::string> valResult = m_validatorExtern.validate(whiteList, unvalidated, validated, getActualTimestamp());
                KARABO_LOG_DEBUG << "Validated reconfiguration:\n" << validated;
                return valResult;
            }

            void applyReconfiguration(const karabo::util::Hash& reconfiguration) {

                karabo::util::Hash instanceInfoUpdate;
                {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);

                    boost::optional<const karabo::util::Hash::Node&> node = reconfiguration.find("archive");
                    if (node && node->getValue<bool>() != m_parameters.get<bool>("archive")) {
                        instanceInfoUpdate.set("archive", node->getValue<bool>());
                    }
                    node = reconfiguration.find("visibility");
                    if (node && node->getValue<int>() != m_parameters.get<int>("visibility")) {
                        instanceInfoUpdate.set("visibility", node->getValue<int>());
                    }

                    m_parameters.merge(reconfiguration);
                }

                if (!instanceInfoUpdate.empty()) {
                    updateInstanceInfo(instanceInfoUpdate);
                }

                KARABO_LOG_DEBUG << "After user interaction:\n" << reconfiguration;
                if (m_validatorExtern.hasReconfigurableParameter())
                    emit("signalStateChanged", reconfiguration, getInstanceId());
                else
                    emit("signalChanged", reconfiguration, getInstanceId());

            }

            void slotKillDevice() {
                // It is important to know who gave us the kill signal
                std::string senderId = getSenderInfo("slotKillDevice")->getInstanceIdOfSender();
                this->preDestruction(); // Give devices a chance to react
                this->stopFsm();
                if (senderId == m_serverId) { // Our server killed us
                    KARABO_LOG_INFO << "Device is going down as instructed by server";
                } else { // Someone else wants to see us dead, we should inform our server
                    KARABO_LOG_INFO << "Device is going down as instructed by \"" << senderId << "\"";
                    call(m_serverId, "slotDeviceGone", m_deviceId);
                }
            }

            karabo::util::Schema getStateDependentSchema(const karabo::util::State& state) {
                using namespace karabo::util;
                const std::string& currentState = state.name();
                KARABO_LOG_DEBUG << "call: getStateDependentSchema() for state: " << currentState;
                boost::mutex::scoped_lock lock(m_stateDependentSchemaMutex);
                // Check cache, whether a special set of state-dependent expected parameters was created before
                std::map<std::string, Schema>::iterator it = m_stateDependentSchema.find(currentState);
                if (it == m_stateDependentSchema.end()) { // No
                    it = m_stateDependentSchema.insert(make_pair(currentState, Device::getSchema(m_classId, Schema::AssemblyRules(WRITE, currentState)))).first; // New one
                    KARABO_LOG_DEBUG << "Providing freshly cached state-dependent schema:\n" << it->second;
                    if (!m_injectedSchema.empty()) it->second.merge(m_injectedSchema);
                } else {
                    KARABO_LOG_DEBUG << "Schema was already cached";
                }
                return it->second;
            }

            void updateLatencies(const karabo::util::Hash::Pointer& performanceMeasures) {
                if (this->get<bool>("performanceStatistics.enable")) {
                    // Keys and values of 'performanceMeasures' are defined in SignalSlotable::updatePerformanceStatistics
                    // and expectedParameters has to foresee this content under node "performanceStatistics".
                    this->set(karabo::util::Hash("performanceStatistics", *performanceMeasures));
                }
            }

            void onBrokerError(const std::string& message) {
                KARABO_LOG_ERROR << "Broker consumption problem: " << message;
                // Trigger alarm, but not always a new one (system is busy anyway). By setting messagingProblems
                // up to every second, we can investigate roughly the time of problems via the data logger.
                if (!get<bool>("performanceStatistics.messagingProblems")
                    || (karabo::util::Epochstamp() - m_lastBrokerErrorStamp).getTotalSeconds() >= 1ull) {
                    set(karabo::util::Hash("performanceStatistics.messagingProblems", true));
                    m_lastBrokerErrorStamp.now();
                }
            }

            const std::pair<bool, const karabo::util::AlarmCondition> evaluateAndUpdateAlarmCondition(bool forceUpdate,
                                                                                                      const karabo::util::Hash& previousParametersInAlarm,
                                                                                                      bool silent) {
                using namespace karabo::util;
                if (m_validatorIntern.hasParametersInWarnOrAlarm()) {
                    const Hash& h = m_validatorIntern.getParametersInWarnOrAlarm();
                    std::vector<AlarmCondition> v;

                    v.push_back(m_globalAlarmCondition);

                    for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
                        using std::string;
                        const Hash& desc = it->getValue<Hash>();
                        const string& type = desc.get<string>("type");
                        if (!silent) {
                            const string& propertyName = it->getKey();
                            // log message only if alarm is new on property or of new type
                            if (!previousParametersInAlarm.has(propertyName)
                                || previousParametersInAlarm.get<string>(propertyName + ".type") != type) {
                                    KARABO_LOG_WARN << type << ": " << desc.get<string>("message");
                                }
                        }
                        v.push_back(AlarmCondition::fromString(type));
                    }
                    return std::pair<bool, const AlarmCondition > (true, AlarmCondition::returnMostSignificant(v));
                } else if (forceUpdate) {
                    return std::pair<bool, const AlarmCondition > (true, m_globalAlarmCondition);
                }
                return std::pair<bool, const AlarmCondition > (false, AlarmCondition::NONE);
            }

            /**
             * Evaluates difference between previous and current parameter in alarm conditions and emits
             * a signal with the update
             *
             * @param previous: alarm conditions previously present on the device.
             * @param forceUpdate: force updating alarms even if no change occurred on validator side.
             *
             * Note: calling this method must be protected by a state change mutex!
             */
            void evaluateAlarmUpdates(const karabo::util::Hash& previous, karabo::util::Hash& result, bool forceUpdate = false) {
                using namespace karabo::util;


                Hash& toClear = result.bindReference<Hash>("toClear");
                Hash& toAdd = result.bindReference<Hash>("toAdd");
                std::map<std::string, std::unordered_set<std::string> > knownAlarms; //alarms already known to the system which have not updated

                const Hash& current = m_validatorIntern.getParametersInWarnOrAlarm();
                if (!previous.empty()) {
                    for (Hash::const_iterator it = previous.begin(); it != previous.end(); ++it) {
                        const boost::optional<const Hash::Node&> currentEntry = current.find(it->getKey());
                        const Hash& desc = it->getValue<Hash>();
                        const std::string& exType = desc.get<std::string>("type");
                        if (currentEntry && exType == currentEntry->getValue<Hash>().get<std::string>("type")) {
                            if (!forceUpdate // on force update we don't care if timestamps match
                                && (Timestamp::fromHashAttributes(it->getAttributes())
                                    == Timestamp::fromHashAttributes(currentEntry->getAttributes()))) {
                                knownAlarms[it->getKey()].insert(exType);
                            }

                            continue; //alarmCondition still exists nothing to clean

                        }
                        //add simple entry to allow for cleaning
                        const std::string& property = it->getKey();

                        boost::optional<Hash::Node&> typesListN = toClear.find(property);
                        if (!typesListN) {
                            typesListN = toClear.set(property, std::vector<std::string>());
                        }
                        std::vector<std::string>& typesList = typesListN->getValue<std::vector<std::string> >();
                        typesList.push_back(desc.get<std::string>("type"));
                        std::string propertyDotSep(it->getKey());
                        boost::replace_all(propertyDotSep, Validator::kAlarmParamPathSeparator, ".");
                        KARABO_LOG_DEBUG << "Clearing alarm condition " << propertyDotSep << " -> " << desc.get<std::string>("type");
                    }
                }

                //now add new alarms
                for (Hash::const_iterator it = current.begin(); it != current.end(); ++it) {
                    const Hash& desc = it->getValue<Hash>();
                    const std::string& conditionString = desc.get<std::string>("type");
                    // avoid unnecessary chatter of already sent messages.
                    if (forceUpdate || knownAlarms[it->getKey()].find(conditionString) == knownAlarms[it->getKey()].end()) {

                        const AlarmCondition& condition = AlarmCondition::fromString(conditionString);

                        const std::string& property = it->getKey();
                        std::string propertyDotSep(property);
                        boost::replace_all(propertyDotSep, Validator::kAlarmParamPathSeparator, ".");

                        Hash::Node& propertyNode = toAdd.set(property, Hash());
                        Hash::Node& entryNode = propertyNode.getValue<Hash>().set(conditionString, Hash());
                        Hash& entry = entryNode.getValue<Hash>();

                        entry.set("type", conditionString);
                        // We expect to be protected by locking of m_objectStateChangeMutex outside of the current function 
                        entry.set("description", m_fullSchema.getInfoForAlarm(propertyDotSep, condition));
                        entry.set("needsAcknowledging", m_fullSchema.doesAlarmNeedAcknowledging(propertyDotSep, condition));
                        const Timestamp& occuredAt = Timestamp::fromHashAttributes(it->getAttributes());
                        occuredAt.toHashAttributes(entryNode.getAttributes());
                    }
                }

            }

            /**
             * This slot is called by the alarm service when it gets (re-) instantiated. The alarm service will pass
             * any for this instances that it recovered from its persisted data. These should be checked against whether
             * they are still valid and if new ones appeared
             *
             * @param existingAlarms: A hash containing existing alarms pertinent to this device. May be empty.
             */
            void slotReSubmitAlarms(const karabo::util::Hash& existingAlarms) {
                using namespace karabo::util;
                Hash existingAlarmsRF; //reformatted to match format updateAlarmServiceWithParametersInAlarm expects as previous
                for (Hash::const_iterator propIt = existingAlarms.begin(); propIt != existingAlarms.end(); ++propIt) {
                    const Hash& propertyHash = propIt->getValue<Hash>();
                    const std::string& property = propIt->getKey();
                    for (Hash::const_iterator aTypeIt = propertyHash.begin(); aTypeIt != propertyHash.end(); ++aTypeIt) {
                        existingAlarmsRF.set(boost::replace_all_copy(property, ".", Validator::kAlarmParamPathSeparator), Hash("type", aTypeIt->getKey()));
                    }
                }

                Hash alarmsToUpdate;
                {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    evaluateAlarmUpdates(existingAlarmsRF, alarmsToUpdate, true);
                }
                reply(getInstanceId(), alarmsToUpdate);

            }

            /**
             * Updates attributes in the device's runtime schema.
             * @param updates: updated attributes, expected to be of form Hash("instanceId", str, "updates", vector<Hash>) where
             * each entry in updates is of the form Hash("path", str, "attribute", str, "value", valueType)
             *
             * reply is of the form Hash("success" bool, "instanceId", str, "updatedSchema", Schema, "requestedUpdate", vector<Hash>)
             * where success indicates a successful update, instanceId the device that performed the update
             * updatedSchema the new valid schema, regardless of success or not, and requestedUpdates the
             * original update request, as received through onUpdateAttributes
             */
            void slotUpdateSchemaAttributes(const std::vector<karabo::util::Hash>& updates) {
                bool success = false;
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                try {
                    success = m_fullSchema.applyRuntimeUpdates(updates);
                    // Notify the distributed system
                    emit("signalSchemaUpdated", m_fullSchema, m_deviceId);

                } catch (...) {
                    success = false;

                }
                reply(karabo::util::Hash("success", success, "instanceId", getInstanceId(), "updatedSchema", m_fullSchema, "requestedUpdate", updates));
            }

            /**
             * Clear any lock on this device
             */
            void slotClearLock() {
                set("lockedBy", std::string());
            }

            /**
             * Internal helper for slotTimeTick
             */
            void onTimeUpdateHelper(unsigned long long id, unsigned long long sec, unsigned long long frac, unsigned long long period) {
                // This ensure that onTimeUpdate is called with strong monotonically increasing ids:
                // If some where missing, call with them now - with same time stamp...
                // If this is called with an id smaller or equal than before, it just skips calling onTimeUpdate with
                // this id since that has already been done.
                // This protection is needed since in DeviceServer::onTimeUpdate directly calls
                // (Base)Device::slotTimeTick with the guarantee to be sequential, but then this helper is posted on
                // the event loop which does not guarantee to keep the order (and in praxis ids are rarely swapped!).
                // Another solution would be to use one boost::asio::io_service::strand per Device in DeviceServer,
                // post slotTimeTick through that and do not post again in Device::slotTimeTick.

                // Protect since several onTimeUpdateHelper could run in parallel:
                boost::mutex::scoped_lock lock(m_lastTimeIdUpdatedMutex);

                if (m_lastTimeIdUpdated == 0ull) { // Called the first time
                    m_lastTimeIdUpdated = id - 1ull;
                }
                const unsigned long long largestOnTimeUpdateBacklog = 600000000ull / period; // 6*10^8: 10 min in microsec
                if (id > m_lastTimeIdUpdated + largestOnTimeUpdateBacklog) {
                    // Don't treat an 'id' older than 10 min - for a period of 100 millisec that is 6000 ids in the past
                    KARABO_LOG_WARN << "Big gap between trainIds: from " << m_lastTimeIdUpdated << " to " << id
                            << ". Call hook for time updates only for last " << largestOnTimeUpdateBacklog << " ids.";
                    m_lastTimeIdUpdated = id - largestOnTimeUpdateBacklog;
                }

                while (m_lastTimeIdUpdated < id) {
                    onTimeUpdate(++m_lastTimeIdUpdated, sec, frac, period);
                }
            }
        };


    }
}

#endif
