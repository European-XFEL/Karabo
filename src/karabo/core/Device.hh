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

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <unordered_set>
#include <karabo/net/utils.hh>
#include <karabo/util.hpp>
#include <karabo/util/SignalHandler.hh>
#include <karabo/util/RollingWindowStatistics.hh>
#include <karabo/xms.hpp>
#include <karabo/log/Logger.hh>

#include "coredll.hh"

#include <karabo/util/State.hh>
#include "NoFsm.hh"
#include "DeviceClient.hh"



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

        class BaseDevice : public virtual karabo::xms::SignalSlotable {

        public:

            KARABO_CLASSINFO(BaseDevice, "BaseDevice", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS;

            virtual ~BaseDevice() {
            }

            virtual void run() = 0;

            // TODO
            // Can be removed, if sending current configuration after instantiation by server is deprecated
            virtual karabo::util::Hash getCurrentConfiguration(const std::string& tags = "") const = 0;

        };

        /**
         * The Device class.
         */
        template <class FSM = NoFsm>
        class Device : public BaseDevice, public FSM {

            karabo::util::Validator m_validatorIntern;
            karabo::util::Validator m_validatorExtern;

            boost::shared_ptr<DeviceClient> m_deviceClient;

            std::string m_classId;
            std::string m_serverId;
            std::string m_deviceId;

            std::map<std::string, karabo::util::Schema> m_stateDependendSchema;
            mutable boost::mutex m_stateDependendSchemaMutex;

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

            krb_log4cpp::Category* m_log;

            mutable boost::mutex m_objectStateChangeMutex;
            karabo::util::Hash m_parameters;
            karabo::util::Schema m_staticSchema;
            karabo::util::Schema m_injectedSchema;
            karabo::util::Schema m_fullSchema;

            karabo::util::AlarmCondition m_globalAlarmCondition;




        public:

            KARABO_CLASSINFO(Device, "Device", "1.0")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;

                STRING_ELEMENT(expected).key("compatibility")
                        .displayedName("Compatibility")
                        .description("The compatibility of this device to the Karabo framework")
                        .expertAccess()
                        .readOnly()
                        .initialValue(Device::classInfo().getVersion())
                        .commit();

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

                CHOICE_ELEMENT(expected).key("_connection_")
                        .displayedName("Connection")
                        .description("The connection to the communication layer of the distributed system")
                        .appendNodesOfConfigurationBase<karabo::net::BrokerConnection>()
                        .assignmentOptional().defaultValue("Jms")
                        .adminAccess()
                        .init()
                        .commit();

                INT32_ELEMENT(expected).key("visibility")
                        .displayedName("Visibility")
                        .description("Configures who is allowed to see this device at all")
                        .assignmentOptional().defaultValue(karabo::util::Schema::OBSERVER)
                        .adminAccess()
                        .reconfigurable()
                        .commit();

                STRING_ELEMENT(expected).key("classId")
                        .displayedName("ClassID")
                        .description("The (factory)-name of the class of this device")
                        .expertAccess()
                        .readOnly()
                        .initialValue(Device::classInfo().getClassId())
                        .commit();

                STRING_ELEMENT(expected).key("serverId")
                        .displayedName("ServerID")
                        .description("The device-server on which this device is running on")
                        .expertAccess()
                        .readOnly()
                        .commit();

                STRING_ELEMENT(expected).key("deviceId")
                        .displayedName("DeviceID")
                        .description("The device instance ID uniquely identifies a device instance in the distributed system")
                        .readOnly()
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
                        .adminAccess()
                        .commit();

                STATE_ELEMENT(expected).key("state")
                        .displayedName("State")
                        .description("The current state the device is in")
                        .initialValue(State::UNKNOWN)
                        .commit();

                ALARM_ELEMENT(expected).key("alarmCondition")
                        .displayedName("Alarm condition")
                        .description("The current alarm condition of the device. "
                                     "Evaluates to the highest condition on any"
                                     " property if not set manually.")
                        .initialValue(AlarmCondition::NONE)
                        .commit();

                NODE_ELEMENT(expected).key("performanceStatistics")
                        .displayedName("Performance Statistics")
                        .description("Accumulates some statistics")
                        .expertAccess()
                        .commit();

                BOOL_ELEMENT(expected).key("performanceStatistics.enable")
                        .displayedName("Enable Performance Indicators")
                        .description("Enables some statistics to follow the performance of an individual device")
                        .reconfigurable()
                        .expertAccess()
                        .assignmentOptional().defaultValue(false)
                        .commit();

                FLOAT_ELEMENT(expected).key("performanceStatistics.brokerLatency")
                        .displayedName("Broker latency (ms)")
                        .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                        .description("Average time interval between remote message sending and receiving it on this device before queuing.")
                        .expertAccess()
                        .readOnly().initialValue(0)
                        .commit();

                FLOAT_ELEMENT(expected).key("performanceStatistics.processingLatency")
                        .displayedName("Processing latency (ms)")
                        .description("Average time interval between remote message sending and reading it from the queue on this device.")
                        .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                        .expertAccess()
                        .readOnly().initialValue(0)
                        .warnHigh(3000.f) // 3 s
                        .info("Long average time between message being sent and start of its processing")
                        .needsAcknowledging(false)
                        .alarmHigh(10000.f) // 10 s
                        .info("Very long average time between message being sent and start of its processing")
                        .needsAcknowledging(false)
                        .commit();

                UINT32_ELEMENT(expected).key("performanceStatistics.maxProcessingLatency")
                        .displayedName("Maximum proc. latency")
                        .description("Maximum processing latency within averaging interval.")
                        .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                        .expertAccess()
                        .readOnly().initialValue(0)
                        .commit();

                UINT32_ELEMENT(expected).key("performanceStatistics.messageQueueSize")
                        .displayedName("Local message queue size")
                        .description("Current size of the local message queue.")
                        .expertAccess()
                        .readOnly().initialValue(0)
                        //.warnHigh(100)
                        .commit();

                FSM::expectedParameters(expected);
            }

            Device(const karabo::util::Hash& configuration) : m_errorRegex(".*error.*", boost::regex::icase),
                m_globalAlarmCondition(karabo::util::AlarmCondition::NONE) {

                // Make the configuration the initial state of the device
                m_parameters = configuration;

                m_timeId = 0;
                m_timeSec = 0;
                m_timeFrac = 0;
                m_timePeriod = 0;

                // Set serverId
                if (configuration.has("_serverId_")) configuration.get("_serverId_", m_serverId);
                else m_serverId = KARABO_NO_SERVER;

                // Set instanceId
                if (configuration.has("_deviceId_")) configuration.get("_deviceId_", m_deviceId);
                else m_deviceId = "__none__";

                // Setup the validation classes
                karabo::util::Validator::ValidationRules rules;
                rules.allowAdditionalKeys = true;
                rules.allowMissingKeys = true;
                rules.allowUnrootedConfiguration = true;
                rules.injectDefaults = false;
                rules.injectTimestamps = true;
                m_validatorIntern.setValidationRules(rules);
                m_validatorExtern.setValidationRules(rules);

                // Setup device logger
                m_log = &(karabo::log::Logger::getCategory(m_deviceId)); // TODO use later: "device." + instanceId


            }

            virtual ~Device() {
                KARABO_LOG_FRAMEWORK_TRACE << "Device::~Device() dtor : m_deviceClient.use_count()="
                        << m_deviceClient.use_count() << "\n"
#ifdef __linux__
                        << karabo::util::StackTrace();
#else
                        << "Stack trace not implemented";
#endif

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
             * Updates the state of the device. This function automatically notifies any observers in the distributed system.
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
             * Convenience function for writing data objects that reflect a single element in the data schema.
             * In this case the root-name must not be included in the data object hierarchy but should be given
             * as second argument to this function.
             * Data will be timestamped and send immediately (write/update).
             * @param channelName The output channel name
             * @param key The data element (root-)key
             * @param data Hash::Pointer object
             */
            void writeChannel(const std::string& channelName, const std::string& key, const karabo::util::Hash::Pointer& data) {
                karabo::util::Hash::Pointer root(new karabo::util::Hash(key, data));
                writeChannel(channelName, root);
            }

            /**
             * Writes a data object to the specified channel. The data object internally must
             * follow exactly the data schema as defined in the expected parameters.
             * @param channelName The output channel name
             * @param data Hash::Pointer object
             */
            void writeChannel(const std::string& channelName, karabo::util::Hash::Pointer& data) {
                // TODO think about proper validation and time tagging later
                karabo::xms::OutputChannel::Pointer channel = this->getOutputChannel(channelName);
                const karabo::util::Timestamp& ts = getActualTimestamp();

                for (karabo::util::Hash::iterator it = data->begin(); it != data->end(); ++it) {
                    ts.toHashAttributes(it->getAttributes());
                }
                channel->write(data);
                channel->update();
            }

            void signalEndOfStream(const std::string& channelName) {
                this->getOutputChannel(channelName)->signalEndOfStream();
            }

            /**
             * Updates the state of the device with all key/value pairs given in the hash
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
             */
            void set(const karabo::util::Hash& hash) {
                this->set(hash, getActualTimestamp());
            }

            /**
             * Updates the state of the device with all key/value pairs given in the hash
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
             */
            void set(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp) {
                using namespace karabo::util;

                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                karabo::util::Hash validated;
                std::pair<bool, std::string> result;


                const bool hadPreviousAlarm = m_validatorIntern.hasParametersInWarnOrAlarm();
                Hash previousParametersInAlarm;
                if (hadPreviousAlarm) {
                    previousParametersInAlarm = m_validatorIntern.getParametersInWarnOrAlarm(); //copies on purpose
                }


                result = m_validatorIntern.validate(m_fullSchema, hash, validated, timestamp);

                if (result.first == false) {
                    KARABO_LOG_WARN << "Bad parameter setting attempted, validation reports: " << result.second;
                }

                // Check for parameters being in a bad condition
                std::pair<bool, const AlarmCondition> resultingCondition = this->evaluateAndUpdateAlarmCondition(hadPreviousAlarm);
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

            template <class ValueType>
            void setNoValidate(const std::string& key, const ValueType& value) {
                this->setNoValidate<ValueType>(key, value, getActualTimestamp());
            }

            template <class ValueType>
            void setNoValidate(const std::string& key, const ValueType& value, const karabo::util::Timestamp& timestamp) {
                karabo::util::Hash h(key, value);
                this->setNoValidate(h, timestamp);
            }

            void setNoValidate(const karabo::util::Hash& hash) {
                this->setNoValidate(hash, getActualTimestamp());
            }

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
                            KARABO_PARAMETER_EXCEPTION("State element at " + key + " may only return state objects");
                        }
                        if (leafType == karabo::util::Schema::ALARM_CONDITION) {
                            if (typeid (T) == typeid (karabo::util::AlarmCondition)) {
                                return *reinterpret_cast<const T*> (&karabo::util::AlarmCondition::fromString(m_parameters.get<std::string>(key)));
                            }
                            KARABO_PARAMETER_EXCEPTION("Alarm condition element at " + key + " may only return alarm condition objects");
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
                    m_stateDependendSchema.clear();

                    // Save injected
                    m_injectedSchema.merge(schema);

                    // Merge to full schema
                    m_fullSchema.merge(m_injectedSchema);

                }

                // Notify the distributed system
                emit("signalSchemaUpdated", m_fullSchema, m_deviceId);

                // Merge all parameters
                if (keepParameters) validated.merge(m_parameters);
                set(validated);

                KARABO_LOG_INFO << "Schema updated";
            }

            /**
             * Replace existing schema descriptions by static (hard coded in expectedParameters) part and
             * add additional (dynamic) descriptions
             * @param schema
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
                    m_stateDependendSchema.clear();

                    // Reset fullSchema
                    m_fullSchema = m_staticSchema;

                    // Save injected
                    m_injectedSchema = schema;

                    // Merge to full schema
                    m_fullSchema.merge(m_injectedSchema);
                }

                // Notify the distributed system
                emit("signalSchemaUpdated", m_fullSchema, m_deviceId);

                // Merge all parameters
                if (keepParameters) validated.merge(m_parameters);
                set(validated);

                KARABO_LOG_INFO << "Schema updated";
            }

            void setProgress(const int value, const std::string& associatedText = "") {
                int v = (m_progressMin + value / float(m_progressMax - m_progressMin)) * 100;
                set("progress", v);
            }

            void resetProgress() {
                set("progress", m_progressMin);
            }

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
                if (tags.empty()) return m_parameters;
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                karabo::util::Hash filtered;
                karabo::util::HashFilter::byTag(m_fullSchema, m_parameters, filtered, tags);
                return filtered;
            }

            karabo::util::Hash filterByTags(const karabo::util::Hash& hash, const std::string& tags) const {
                karabo::util::Hash filtered;
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                karabo::util::HashFilter::byTag(m_fullSchema, hash, filtered, tags);
                return filtered;
            }

            const std::string& getServerId() const {
                return m_serverId;
            }

            const karabo::util::State getState() {
                return this->get<karabo::util::State>("state");
            }





            // This function will polymorphically be called by the FSM template

            void updateState(const karabo::util::State& cs) {
                try {
                    const std::string& currentState = cs.name();
                    KARABO_LOG_FRAMEWORK_DEBUG << "updateState(state): \"" << currentState << "\".";
                    if (getState().name() != currentState) {
                        set("state", cs);
                        if (boost::regex_match(currentState, m_errorRegex)) {
                            updateInstanceInfo(karabo::util::Hash("status", "error"));
                        } else {
                            // Reset the error status - protect against non-initialised instanceInfo
                            if (!getInstanceInfo().has("status") || getInstanceInfo().get<std::string>("status") == "error") {
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

            virtual void exceptionFound(const karabo::util::Exception& e) {
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

            virtual void onTimeUpdate(unsigned long long id, unsigned long long sec, unsigned long long frac, unsigned long long period) {
            }

            // TODO:  Implement local call: just post command on local queue

            void execute(const std::string& command) const {
                call("", command);
            }

            template <class A1>
            void execute(const std::string& command, const A1& a1) const {
                call("", command, a1);
            }

            template <class A1, class A2>
            void execute(const std::string& command, const A1& a1, const A2& a2) const {
                call("", command, a1, a2);
            }

            template <class A1, class A2, class A3>
            void execute(const std::string& command, const A1& a1, const A2& a2, const A3& a3) const {
                call("", command, a1, a2, a3);
            }

            template <class A1, class A2, class A3, class A4>
            void execute(const std::string& command, const A1& a1, const A2& a2, const A3& a3, const A4& a4) const {
                call("", command, a1, a2, a3, a4);
            }

            const karabo::util::AlarmCondition & getAlarmCondition() const {
                return karabo::util::AlarmCondition::fromString(this->get<std::string>("alarmCondition"));
            }

            void setAlarmCondition(const karabo::util::AlarmCondition & condition) {

                using namespace karabo::util;

                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                m_globalAlarmCondition = condition;
                std::pair<bool, const AlarmCondition> result = this->evaluateAndUpdateAlarmCondition(true);
                if (result.first && result.second.asString() != m_parameters.get<std::string>("alarmCondition")) {
                    lock.unlock();
                    Hash h;
                    h.set("alarmCondition", result.second.asString()).setAttribute(KARABO_INDICATE_ALARM_SET, true);
                    //also set the fields attribute to this condition
                    this->setNoValidate("alarmCondition", h);
                    this->m_parameters.setAttribute("alarmCondition", KARABO_ALARM_ATTR, result.second.asString());
                }

            }

            const karabo::util::AlarmCondition & getAlarmCondition(const std::string & key, const std::string & sep = ".") const {
                const std::string & propertyCondition = this->m_parameters.template getAttribute<std::string>(key, KARABO_ALARM_ATTR, sep);
                return karabo::util::AlarmCondition::fromString(propertyCondition);
            }

            bool hasRollingStatistics(const std::string & path) const {
                return this->getFullSchema().hasRollingStatistics(path);
            }

            karabo::util::RollingWindowStatistics::ConstPointer getRollingStatistics(const std::string & path) const {
                return m_validatorIntern.getRollingStatistics(path);
            }

            const karabo::util::Hash getAlarmInfo() const {
                /**
                 * Returns a hash containing the info field information
                 * for current alarms on the device
                 *
                 * @return a hash with structure key: path to property -> alarm info (string)
                 */
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




        protected: // Functions and Classes

            virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration) {
            }

            virtual void postReconfigure() {
            }

            virtual void preDestruction() {
            }

        private: // Functions

            /**
             * This function will typically be called by the DeviceServer (or directly within the startDevice application).
             * The call to run is blocking and afterwards communication should happen only via call-backs
             */
            void run() {

                if (!m_connection) {
                    m_connectionInjected = false;

                    // Instantiate connection
                    karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::createChoice("_connection_", m_parameters);

                    // Initialize the SignalSlotable instance
                    init(m_deviceId, connection);
                }

                // Initialize FSM slots (the interface of this function must be inherited from the templated FSM)
                this->initFsmSlots(); // requires template CONCEPT

                // Initialize Device slots
                this->initDeviceSlots();

                // Register guard for slot calls
                this->registerSlotCallGuardHandler(boost::bind(&karabo::core::Device<FSM>::slotCallGuard, this, _1));

                // Register exception handler
                this->registerExceptionHandler(boost::bind(&karabo::core::Device<FSM>::exceptionFound, this, _1));

                // Register updateLatencies handler
                this->registerPerformanceStatisticsHandler(boost::bind(&karabo::core::Device<FSM>::updateLatencies,
                                                                       this, _1, _2, _3, _4, _5));

                // This initializations or done here and not in the constructor as they involve virtual function calls
                this->initClassId();
                this->initSchema();

                // Prepare some info further describing this particular instance
                // status, visibility, owner, lang
                karabo::util::Hash instanceInfo;
                instanceInfo.set("type", "device");
                instanceInfo.set("classId", m_classId);
                instanceInfo.set("serverId", m_serverId);
                instanceInfo.set("visibility", this->get<int >("visibility"));
                instanceInfo.set("compatibility", Device::classInfo().getVersion());
                instanceInfo.set("host", net::bareHostName());
                instanceInfo.set("status", "ok");
                instanceInfo.set("archive", this->get<bool>("archive"));

                boost::thread t(boost::bind(&karabo::core::Device<FSM>::runEventLoop, this, this->get<int>("heartbeatInterval"), instanceInfo));

                boost::this_thread::sleep(boost::posix_time::milliseconds(100));

                bool ok = ensureOwnInstanceIdUnique();
                if (!ok) {
                    t.join(); // Blocks
                    return;
                }

                KARABO_LOG_INFO << "'" << m_classId << "' with deviceId: '" << this->getInstanceId() << "' got started"
                        << " on server '" << this->getServerId() << "'.";

                // ClassId
                m_parameters.set("classId", m_classId);
                // DeviceId
                m_parameters.set("deviceId", m_deviceId);
                // ServerId
                m_parameters.set("serverId", m_serverId);

                // Validate first time to assign timestamps
                {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);

                    // The following lines of code are needed to initially inject timestamps to the parameters
                    karabo::util::Hash validated;
                    std::pair<bool, std::string> result = m_validatorIntern.validate(m_fullSchema, m_parameters, validated, getActualTimestamp());
                    if (result.first == false) KARABO_LOG_WARN << "Bad parameter setting attempted, validation reports: " << result.second;
                    m_parameters.merge(validated, karabo::util::Hash::REPLACE_ATTRIBUTES);
                }

                // Instantiate all channels
                this->initChannels();
                this->connectInputChannels();


                // Start the state machine
                this->startFsm(); // This function must be inherited from the templated base class (it's a concept!)

                if (m_parameters.get<bool>("useTimeserver")) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Connecting to time server";
                    connect("Karabo_TimeServer", "signalTimeTick", "", "slotTimeTick");
                }



                t.join(); // Blocks
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

                KARABO_SYSTEM_SIGNAL4("signalNotification", string /*type*/, string /*messageShort*/, string /*messageDetail*/, string /*deviceId*/);

                KARABO_SYSTEM_SIGNAL2("signalSchemaUpdated", karabo::util::Schema /*deviceSchema*/, string /*deviceId*/);

                KARABO_SIGNAL2("signalAlarmUpdate", std::string, karabo::util::Hash);


                KARABO_SLOT(slotReconfigure, karabo::util::Hash /*reconfiguration*/)
                KARABO_SLOT(slotGetConfiguration)
                KARABO_SLOT(slotGetSchema, bool /*onlyCurrentState*/);
                KARABO_SLOT(slotKillDevice)
                KARABO_SLOT(slotTimeTick, unsigned long long /*id */, unsigned long long /* sec */, unsigned long long /* frac */, unsigned long long /* period */);
                KARABO_SLOT(slotReSubmitAlarms, karabo::util::Hash);
                KARABO_SLOT(slotUpdateSchemaAttributes, std::vector<karabo::util::Hash>);

            }

            /**
             *  Called in beginning of run() to setup p2p channels, will
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
                            createOutputChannel(key, m_parameters);
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

            bool slotCallGuard(const std::string& slotName) {
                using namespace karabo::util;
                std::vector<State> allowedStates;
                {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    if (m_fullSchema.has(slotName) && m_fullSchema.hasAllowedStates(slotName)) {
                        allowedStates = m_fullSchema.getAllowedStates(slotName);
                    }
                }

                if (!allowedStates.empty()) {
                    const State& currentState = getState();
                    if (std::find(allowedStates.begin(), allowedStates.end(), currentState) == allowedStates.end()) {
                        std::ostringstream msg;
                        msg << "Command " << "\"" << slotName << "\"" << " is not allowed in current state "
                                << "\"" << currentState.name() << "\" of device " << "\"" << m_deviceId << "\".";
                        std::string errorMessage = msg.str();
                        reply(errorMessage);
                        return false;
                    }
                }
                return true;
            }

            void slotGetConfiguration() {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                reply(m_parameters, m_deviceId);
            }

            void slotGetSchema(bool onlyCurrentState) {
                if (onlyCurrentState) {
                    const karabo::util::State& currentState = getState();
                    const karabo::util::Schema schema(getStateDependentSchema(currentState));
                    reply(schema, m_deviceId);
                } else {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    reply(m_fullSchema, m_deviceId);
                }
            }

            void slotReconfigure(const karabo::util::Hash& newConfiguration) {
                if (newConfiguration.empty()) return;
                std::pair<bool, std::string > result;
                try {
                    karabo::util::Hash validated;

                    result = validate(newConfiguration, validated);

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
                    }
                } catch (const karabo::util::Exception& e) {
                    this->exceptionFound(e);
                    reply(false, e.userFriendlyMsg());
                    return;
                }
                reply(result.first, result.second);
            }

            std::pair<bool, std::string> validate(const karabo::util::Hash& unvalidated, karabo::util::Hash& validated) {
                // Retrieve the current state of the device instance
                const karabo::util::State& currentState = getState();
                const karabo::util::Schema whiteList(getStateDependentSchema(currentState));
                KARABO_LOG_DEBUG << "Incoming (un-validated) reconfiguration:\n" << unvalidated;
                std::pair<bool, std::string> valResult = m_validatorExtern.validate(whiteList, unvalidated, validated);
                KARABO_LOG_DEBUG << "Validated reconfiguration:\n" << validated;
                return valResult;
            }

            void applyReconfiguration(const karabo::util::Hash& reconfiguration) {

                {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    m_parameters.merge(reconfiguration);
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
                if (senderId == m_serverId) { // Our server killed us
                    KARABO_LOG_INFO << "Device is going down as instructed by server";
                } else { // Someone else wants to see us dead, we should inform our server
                    KARABO_LOG_INFO << "Device is going down as instructed by \"" << senderId << "\"";
                    call(m_serverId, "slotDeviceGone", m_deviceId);
                }
                this->preDestruction(); // Give devices a chance to react
                this->stopFsm();
                stopEventLoop();
            }

            karabo::util::Schema getStateDependentSchema(const karabo::util::State& state) {
                using namespace karabo::util;
                const std::string& currentState = state.name();
                KARABO_LOG_DEBUG << "call: getStateDependentSchema() for state: " << currentState;
                boost::mutex::scoped_lock lock(m_stateDependendSchemaMutex);
                // Check cache, whether a special set of state-dependent expected parameters was created before
                std::map<std::string, Schema>::iterator it = m_stateDependendSchema.find(currentState);
                if (it == m_stateDependendSchema.end()) { // No
                    it = m_stateDependendSchema.insert(make_pair(currentState, Device::getSchema(m_classId, Schema::AssemblyRules(WRITE, currentState)))).first; // New one
                    KARABO_LOG_DEBUG << "Providing freshly cached state-dependent schema:\n" << it->second;
                    if (!m_injectedSchema.empty()) it->second.merge(m_injectedSchema);
                } else {
                    KARABO_LOG_DEBUG << "Schema was already cached";
                }
                return it->second;
            }

            void updateLatencies(float avgBrokerLatency, unsigned int /*maxBrokerLatency*/,
                                 float avgProcessingLatency, unsigned int maxProcessingLatency,
                                 unsigned int messageQueueSize) {

                // updateLatencies
                if (this->get<bool>("performanceStatistics.enable")) {

                    using karabo::util::Hash;
                    const Hash h("performanceStatistics", Hash("brokerLatency", avgBrokerLatency,
                                                               "processingLatency", avgProcessingLatency,
                                                               "maxProcessingLatency", maxProcessingLatency,
                                                               "messageQueueSize", messageQueueSize));
                    this->set(h);
                }
            }

            void slotTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac, unsigned long long period) {
                {
                    boost::mutex::scoped_lock lock(m_timeChangeMutex);
                    m_timeId = id;
                    m_timeSec = sec;
                    m_timeFrac = frac;
                    m_timePeriod = period;
                }

                onTimeUpdate(id, sec, frac, period);
            }

            karabo::util::Timestamp getActualTimestamp() {
                karabo::util::Epochstamp epochNow;
                unsigned long long id = 0;
                {
                    boost::mutex::scoped_lock lock(m_timeChangeMutex);
                    if (m_timePeriod > 0) {
                        karabo::util::Epochstamp epochLastReceived(m_timeSec, m_timeFrac);
                        karabo::util::TimeDuration duration = epochNow.elapsed(epochLastReceived);
                        unsigned int nPeriods = (duration.getTotalSeconds() * 1000000 + duration.getFractions(karabo::util::MICROSEC)) / m_timePeriod;
                        id = m_timeId + nPeriods;
                    }
                }
                return karabo::util::Timestamp(epochNow, karabo::util::Trainstamp(id));
            }

            const std::pair<bool, const karabo::util::AlarmCondition> evaluateAndUpdateAlarmCondition(bool forceUpate) {
                using namespace karabo::util;
                if (m_validatorIntern.hasParametersInWarnOrAlarm()) {
                    const Hash& h = m_validatorIntern.getParametersInWarnOrAlarm();
                    std::vector<AlarmCondition> v;

                    v.push_back(m_globalAlarmCondition);

                    for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
                        const Hash& desc = it->getValue<Hash>();
                        KARABO_LOG_WARN << desc.get<string>("type") << ": " << desc.get<string>("message");
                        emit("signalNotification", desc.get<string>("type"), desc.get<string>("message"), string(), m_deviceId);
                        v.push_back(AlarmCondition::fromString(desc.get<string>("type")));
                    }
                    return std::pair<bool, const AlarmCondition > (true, AlarmCondition::returnMostSignificant(v));
                } else if (forceUpate) {
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
                std::unordered_set<std::string> knownAlarms; //alarms already known to the system which have not updated

                const Hash& current = m_validatorIntern.getParametersInWarnOrAlarm();
                if (!previous.empty()) {
                    for (Hash::const_iterator it = previous.begin(); it != previous.end(); ++it) {
                        const boost::optional<const Hash::Node&> currentEntry = current.find(it->getKey());
                        if (currentEntry) {
                            if (!forceUpdate) { // on force update we don't care if timestamps match
                                const Timestamp previousTimeStamp = Timestamp::fromHashAttributes(it->getAttributes());
                                const Timestamp currentTimeStamp = Timestamp::fromHashAttributes(currentEntry->getAttributes());
                                if (previousTimeStamp.getTrainId() == currentTimeStamp.getTrainId() &&
                                    previousTimeStamp.getSeconds() == currentTimeStamp.getSeconds() &&
                                    previousTimeStamp.getFractionalSeconds() == previousTimeStamp.getFractionalSeconds()) {
                                    knownAlarms.insert(it->getKey());
                                }
                            }
                            continue; //alarmCondition still exists nothing to clean
                        }
                        //add simple entry to allow for cleaning
                        const Hash& desc = it->getValue<Hash>();
                        const std::string& property = it->getKey();

                        boost::optional<Hash::Node&> typesListN = toClear.find(property);
                        if (!typesListN) {
                            typesListN = toClear.set(property, std::vector<std::string>());
                        }
                        std::vector<std::string>& typesList = typesListN->getValue<std::vector<std::string> >();
                        typesList.push_back(desc.get<std::string>("type"));
                        KARABO_LOG_DEBUG << "Clearing alarm condition " << it->getKey() << " -> " << desc.get<std::string>("type");
                    }
                }

                //now add new alarms
                for (Hash::const_iterator it = current.begin(); it != current.end(); ++it) {
                    // avoid unnecessary chatter of already sent messages.
                    if (forceUpdate || knownAlarms.find(it->getKey()) == knownAlarms.end()) {
                        const Hash& desc = it->getValue<Hash>();
                        const std::string& conditionString = desc.get<std::string>("type");
                        const AlarmCondition& condition = AlarmCondition::fromString(conditionString);

                        const std::string& property = it->getKey();
                        std::string propertyDotSep(property);
                        boost::replace_all(propertyDotSep, Validator::kAlarmParamPathSeparator, ".");

                        Hash::Node& propertyNode = toAdd.set(property, Hash());
                        Hash::Node& entryNode = propertyNode.getValue<Hash>().set(conditionString, Hash());
                        Hash& entry = entryNode.getValue<Hash>();

                        entry.set("type", conditionString);
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
                try {
                    boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                    success = m_fullSchema.applyRuntimeUpdates(updates);
                    // Notify the distributed system
                    emit("signalSchemaUpdated", m_fullSchema, m_deviceId);

                } catch (...) {
                    success = false;

                }
                reply(karabo::util::Hash("success", success, "instanceId", getInstanceId(), "updatedSchema", m_fullSchema, "requestedUpdate", updates));
            }



        };


    }
}

#endif
