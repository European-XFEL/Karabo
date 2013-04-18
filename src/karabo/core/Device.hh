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
#include <karabo/util/Time.hh>
#include <karabo/util/Factory.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/log/Logger.hh>
#include "coredll.hh"

#include "FsmMacros.hh"
#include "FsmBase.hh"
//#include "DeviceClient.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace core {

        // Convenient logging
        #define KARABO_LOG_DEBUG log() << log4cpp::Priority::DEBUG 
        #define KARABO_LOG_INFO  log() << log4cpp::Priority::INFO 
        #define KARABO_LOG_WARN  log() << log4cpp::Priority::WARN 
        #define KARABO_LOG_ERROR log() << log4cpp::Priority::ERROR 

        /**
         * The Device class.
         */
        template <class FSM = FsmBase>
        class Device : public virtual karabo::xms::SignalSlotable, public FSM {

            karabo::util::Validator m_validatorIntern;
            karabo::util::Validator m_validatorExtern;

            //boost::shared_ptr<DeviceClient> m_deviceClient;

            karabo::util::Hash m_instanceInfo;
            
            std::string m_classId;
            std::string m_devSrvInstId;

            std::map<std::string, karabo::util::Schema> m_stateDependendSchema;
            boost::mutex m_stateDependendSchemaMutex;

            boost::mutex m_objectStateChangeMutex;

            // progressBar related
            int m_progressMin;
            int m_progressMax;

            log4cpp::Category* m_log;

            karabo::util::Hash m_parameters;
            karabo::util::Schema m_staticSchema;
            karabo::util::Schema m_injectedSchema;
            karabo::util::Schema m_fullSchema;

        public:

            KARABO_CLASSINFO(Device, "Device", "0.1")
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;
                
                STRING_ELEMENT(expected).key("version")
                        .displayedName("Version")
                        .description("The version of this device class")
                        .advanced()
                        .readOnly()
                        .initialValue(Device::classInfo().getVersion())
                        .commit();

                CHOICE_ELEMENT(expected).key("connection")
                        .displayedName("Connection")
                        .description("The connection to the communication layer of the distributed system")
                        .appendNodesOfConfigurationBase<karabo::net::BrokerConnection>()
                        .assignmentOptional().defaultValue("Jms")
                        .advanced()
                        .init()
                        .commit();
                
                STRING_ELEMENT(expected).key("classId")
                        .displayedName("ClassID")
                        .description("The (factory)-name of the class of this device")
                        .advanced()
                        .readOnly()
                        .commit();         

                STRING_ELEMENT(expected).key("serverId")
                        .displayedName("ServerID")
                        .description("The device-server on which this device is running on")
                        .advanced()
                        .assignmentOptional().noDefaultValue()
                        .init()
                        .commit();

                STRING_ELEMENT(expected).key("instanceId")
                        .displayedName("InstanceID")
                        .description("The device instance ID uniquely identifies a device instance in the distributed system")
                        .assignmentOptional().noDefaultValue()
                        .init()
                        .commit();

                STRING_ELEMENT(expected).key("state")
                        .displayedName("State")
                        .description("The current state the device is in")
                        .readOnly()
                        .initialValue("uninitialized")
                        .commit();

                FSM::expectedParameters(expected);
            }

            Device(const karabo::util::Hash& configuration) {
                using namespace karabo::util;

                try {
                    // Speed access to own classId
                    m_classId = getClassInfo().getClassId();

                    // Make the configuration the initial state of the device
                    m_parameters = configuration;

                    // The static schema is the regular schema as assembled by parsing the expectedParameters functions
                    m_staticSchema = getSchema(m_classId, karabo::util::Schema::AssemblyRules(INIT | WRITE | READ));

                    // If no runtime schema got injected, the full schema equals the static schema
                    m_fullSchema = m_staticSchema;

                    // Set up the validator
                    Validator::ValidationRules rules;
                    rules.allowAdditionalKeys = false;
                    rules.allowMissingKeys = true;
                    rules.allowUnrootedConfiguration = true;
                    rules.injectDefaults = false;
                    rules.injectTimestamps = true;
                    m_validatorIntern.setValidationRules(rules);

                    rules.injectTimestamps = false;
                    m_validatorExtern.setValidationRules(rules);

                    // Speed access to device-server instance
                    if (configuration.has("serverId")) {
                        configuration.get("serverId", m_devSrvInstId);
                    } else {
                        m_devSrvInstId = "__none__";
                    }

                    // Set device instance 
                    string devInstId;
                    if (configuration.has("instanceId")) {
                        configuration.get("instanceId", devInstId);
                    } else { // No instanceId given
                        devInstId = "__none__"; // TODO generate uuid
                    }

                    // Setup device logger
                    m_log = &(karabo::log::Logger::getLogger(devInstId)); // TODO use later: "device." + devInstId

                    // Instantiate connection
                    karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::createChoice("connection", configuration);

                    // Prepare some info further describing this particular instance
                    Hash info("type", "device", "classId", m_classId, "serverId", m_devSrvInstId, "version", Device::classInfo().getVersion(), "host", boost::asio::ip::host_name());
                    
                    // Initialize the SignalSlotable instance
                    init(connection, devInstId, info);

                    // Initialize FSM slots (the interface of this function must be inherited from the templated FSM)
                    this->initFsmSlots(); // requires template CONCEPT

                    // Initialize Device slots
                    this->initDeviceSlots();

                    KARABO_LOG_INFO << "Starting up " << m_classId << " on networkId " << getInstanceId();

                    set("classId", m_classId);
                    
                    


                } catch (const Exception& e) {
                    KARABO_RETHROW;
                }
            }
            
            virtual ~Device() {
            };
            
            /**
             * This function will typically be called by the DeviceServer (or directly within the startDevice application).
             * It must be overridden in any derived device class and typically should block the main thread - bringing
             * the device into an event driven mode of operation.
             */
            virtual void run() {
                boost::thread t(boost::bind(&karabo::core::Device<FSM>::runEventLoop, this, true));
                this->startFsm();
                t.join();
            }

            /**
             * This function allows to communicate to other (remote) devices.
             * Any device contains also a controller for other devices (DeviceClient) 
             * which is returned by this function.
             * 
             * @return DeviceClient instance
             */
            //            DeviceClient& remote() {
            //                if (!m_deviceClient) {
            //                    // Initialize an embedded device client (for composition)
            //                    m_deviceClient = boost::shared_ptr<DeviceClient > (new DeviceClient(shared_from_this()));
            //                }
            //                return *(m_deviceClient);
            //            }

            /**
             * Updates the state of the device. This function automatically notifies any observers in the distributed system.
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @param value The corresponding value (of corresponding data-type)
             */
            template <class ValueType>
            void set(const std::string& key, const ValueType& value, const karabo::util::Timestamp& timestamp = karabo::util::Timestamp()) {
                karabo::util::Hash h(key, value);
                this->set(h, timestamp);
            }

            /**
             * Updates the state of the device with all key/value pairs given in the hash
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
             */
            void set(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp = karabo::util::Timestamp()) {
                using namespace karabo::util;
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);

                karabo::util::Hash validated;
                std::pair<bool, std::string> result = m_validatorIntern.validate(m_fullSchema, hash, validated, timestamp);
                if (result.first == false) {
                    KARABO_LOG_WARN << "Bad parameter setting attempted, validation reports: " << result.second;
                }

                // Check for parameters being in a bad condition
                if (m_validatorIntern.hasParametersInWarnOrAlarm()) {
                    const Hash& h = m_validatorIntern.getParametersInWarnOrAlarm();
                    for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
                        KARABO_LOG_WARN << it->getValue<Hash>().get<string>("message");
                        // TODO trigger warnOrAlarm
                    }
                }

                if (!validated.empty()) {
                    m_parameters.merge(validated);
                    emit("signalChanged", validated, getInstanceId(), m_classId);
                }
            }

            /**
             * Retrieves the current value of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            const T& get(const std::string& key) {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                try {
                    return m_parameters.get<T>(key);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst retrieving parameter (" + key + ") from device"));
                }
            }

            /**
             * Retrieves the current value of any device parameter (that was defined in the expectedParameters function)
             * The value is casted on the fly into the desired type. 
             * NOTE: This function is considerably slower than the simple get() functionality
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            T getAs(const std::string& key) {
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
            log4cpp::Category& log() {
                return (*m_log);
            }

            /**
             * Retrieves all expected parameters of this device
             * @return Schema object containing all expected parameters
             */
            const karabo::util::Schema& getFullSchema() const {
                return m_fullSchema;
            }

            //            /**
            //             * Add external schema descriptions to current schema containers
            //             * @param schema
            //             */
            //            void appendSchema(const karabo::util::Schema& schema) {
            //
            //                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
            //                m_stateDependendSchema.clear();
            //                m_injectedSchema.merge(schema);
            //            }

            /**
             * Replace existing schema descriptions by static (hard coded in expectedParameters) part and
             * add additional (dynamic) descriptions
             * @param schema
             */
            void updateSchema(const karabo::util::Schema& schema) {

                std::cout << "Update Schema requested" << std::endl;
                injectSchema(schema);
                std::cout << "Injected..." << std::endl;
                // Notify the distributed system
                std::string serializedSchema;
                karabo::io::TextSerializer<karabo::util::Schema>::create("Xsd", karabo::util::Hash("indentation", -1))->save(getFullSchema(), serializedSchema);
                std::cout << "Serialized..." << std::endl;
                emit("signalSchemaUpdated", serializedSchema, getInstanceId(), m_classId);
                KARABO_LOG_INFO << "Schema updated";
            }

            void setProgress(const int value, const std::string& associatedText = "") {
                int v = m_progressMin + value / (m_progressMax - m_progressMin);
                emit("signalProgressUpdated", v, associatedText, getInstanceId());
            }

            void resetProgress() {

                emit("signalProgressUpdated", m_progressMin, "");
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
            const AliasType& getAliasFromKey(const std::string& key) {
                try {
                    return m_fullSchema.getAliasFromKey<AliasType>(key);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst retrieving alias from parameter (" + key + ")"));
                }
            }

            /**
             * Converts a device parameter key into its aliased key (must be defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return Aliased representation of the parameter
             * @deprecated
             */
            template <class T>
            const T& key2alias(const std::string& key) {
                return getAliasFromKey(key);
            }

            /**
             * Converts a device parameter alias into the original key (must be defined in the expectedParameters function)
             * @param key A valid parameter-alias of the device (must be defined in the expectedParameters function)
             * @return The original name of the parameter
             */
            template <class AliasType>
            const std::string& getKeyFromAlias(const AliasType& alias) {
                try {
                    return m_fullSchema.getKeyFromAlias(alias);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst retrieving parameter from alias (" + karabo::util::toString(alias) + ")"));
                }
            }

            /**
             * Converts a device parameter alias into the original key (must be defined in the expectedParameters function)
             * @param key A valid parameter-alias of the device (must be defined in the expectedParameters function)
             * @return The original name of the parameter
             * @deprecated use getKeyFromAlias instead
             */
            template <class T>
            const std::string& alias2key(const T& alias) {
                return getKeyFromAlias(alias);
            }

            /**
             * Checks if the argument is a valid alias of some key, i.e. defined in the expectedParameters function
             * @param alias Arbitrary argument of arbitrary type
             * @return true if it is an alias found in one of three containers of parameters:
             * "reconfigurable", "initial" or "monitored",  otherwise false
             */
            template <class T>
            const bool aliasHasKey(const T& alias) {
                return m_fullSchema.aliasHasKey(alias);
            }

            /**
             * Checks if some alias is defined for the given key
             * @param key in expectedParameters mapping
             * @return true if the alias exists
             */
            bool keyHasAlias(const std::string& key) {
                return m_fullSchema.keyHasAlias(key);
            }

            /**
             * Checks the type of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return The enumerated internal reference type of the value
             */
            karabo::util::Types::ReferenceType getValueType(const std::string& key) {
                return m_fullSchema.getValueType(key);
            }

            const karabo::util::Hash& getCurrentConfiguration() const {
                return m_parameters;
            }

            const std::string& getDeviceServerInstanceId() const {
                return m_devSrvInstId;
            }

            

        protected: // Functions and Classes

            void onStateUpdate(const std::string& currentState) { // private
                KARABO_LOG_FRAMEWORK_DEBUG << "onStateUpdate: " << currentState;
                if (get<std::string>("state") != currentState) {
                    set("state", currentState);
                }
                // Reply new state to interested event initiators
                reply(currentState);
            }

            // This function will polymorphically be called by the FSM template

            virtual void errorFoundAction(const std::string& shortMessage, const std::string& detailedMessage) {
                //triggerErrorFound(shortMessage, detailedMessage);
            }

            virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration) {
            }

            virtual void postReconfigure() {
            }

            virtual void preDestruction() {
            }

            //            void triggerErrorFound(const std::string& shortMessage, const std::string& detailedMessage) const;
            //            
            //            void triggerWarning(const std::string& warningMessage, const std::string& priority) const;
            //            
            //            void triggerAlarm(const std::string& alarmMessage, const std::string& priority) const;

        private: // Functions

            void noStateTransition(const std::string& typeId, int state) {
                std::string eventName(typeId);
                boost::regex re(".*\\d+(.+Event).*");
                boost::smatch what;
                bool result = boost::regex_search(typeId, what, re);
                if (result && what.size() == 2) {
                    eventName = what.str(1);
                }
                std::ostringstream msg;
                msg << "Current state of device \"" << m_classId << "\" does not allow any transition for event \"" << eventName << "\"";
                KARABO_LOG_WARN << msg.str();
                emit("signalNoTransition", msg.str(), getInstanceId());
            }

            void initDeviceSlots() {
                using namespace std;

                SIGNAL4("signalErrorFound", string, string, string, string); // timeStamp, shortMsg, longMsg, instanceId
                SIGNAL2("signalBadReconfiguration", string, string); // shortMsg, instanceId 
                SIGNAL2("signalNoTransition", string, string);
                SIGNAL3("signalChanged", karabo::util::Hash, string, string); // changeHash, instanceId, classId

                SIGNAL4("signalWarningOrAlarm", string, string, string, string); // timeStamp, message, type, instanceId
                SIGNAL4("signalWarning", string, string, string, string); // timeStamp, warnMsg, instanceId, priority
                SIGNAL4("signalAlarm", string, string, string, string); // timeStamp, alarmMsg, instanceId, priority

                SIGNAL3("signalSchemaUpdated", string, string, string); // schema, instanceId, classId
                SIGNAL2("signalDeviceInstanceGone", string, string); // DeviceServerInstanceId, deviceInstanceId
                SIGNAL3("signalProgressUpdated", int, string, string); // Progress value [0,100], label, deviceInstanceIdre

                SLOT1(slotReconfigure, karabo::util::Hash)
                SLOT0(slotRefresh)
                SLOT1(slotGetSchema, bool); // onlyCurrentState
                SLOT0(slotKillDeviceInstance)

                // Hard-coded connects (for global slots with this name)
                connectN("", "signalChanged", "*", "slotChanged");
                connectN("", "signalBadReconfiguration", "*", "slotBadReconfiguration");
                connectN("", "signalNoTransition", "*", "slotNoTransition");
                connectN("", "signalErrorFound", "*", "slotErrorFound");
                connectN("", "signalWarning", "*", "slotWarning");
                connectN("", "signalAlarm", "*", "slotAlarm");
                connectN("", "signalSchemaUpdated", "*", "slotSchemaUpdated");
                connectN("", "signalDeviceInstanceGone", "*", "slotDeviceInstanceGone");
                connectN("", "progressUpdate", "*", "slotProgressUpdated");
            }

            void slotRefresh() {
                emit("signalChanged", m_parameters, m_instanceId, m_classId); // TODO: Check whether needed
                //cout << "Refresh " << all << std::endl;
                reply(m_parameters);
            }

            void slotReconfigure(const karabo::util::Hash& newConfiguration) {
                if (newConfiguration.empty()) return;
                karabo::util::Hash validated;
                std::pair<bool, std::string> result = validate(newConfiguration, validated);

                if (result.first == true) { // is a valid reconfiguration
                    // Give device-implementer a chance to specifically react on reconfiguration event by polymorphically calling back
                    try {
                        preReconfigure(validated);
                    } catch (const karabo::util::Exception& e) {
                        this->errorFound(e.userFriendlyMsg(), e.detailedMsg());
                        reply(false, e.userFriendlyMsg());
                        return;
                    }

                    // Merge reconfiguration with current state
                    applyReconfiguration(validated);
                }
                reply(result.first, result.second);
            }

            std::pair<bool, std::string> validate(const karabo::util::Hash& unvalidated, karabo::util::Hash& validated) {
                // Retrieve the current state of the device instance
                const std::string& currentState = get<std::string > ("state");
                karabo::util::Schema& whiteList = getStateDependentSchema(currentState);
                KARABO_LOG_DEBUG << "Incoming (un-validated) reconfiguration:\n" << unvalidated;
                std::pair<bool, std::string> valResult = m_validatorExtern.validate(whiteList, unvalidated, validated);
                KARABO_LOG_DEBUG << "Validated reconfiguration:\n" << validated;
                return valResult;
            }

            void applyReconfiguration(const karabo::util::Hash& reconfiguration) {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                m_parameters.merge(reconfiguration);
                KARABO_LOG_DEBUG << "After user interaction:\n" << reconfiguration;
                emit("signalChanged", reconfiguration, getInstanceId(), m_classId);
                KARABO_LOG_DEBUG << "Current state:\n" << m_parameters;
                this->postReconfigure();
            }

            void slotGetSchema(bool onlyCurrentState) {
                if (onlyCurrentState) {
                    const std::string& currentState = get<std::string > ("state");
                    reply(getStateDependentSchema(currentState));
                } else {
                    reply(m_fullSchema);
                }
            }

            void slotKillDeviceInstance() {
                KARABO_LOG_INFO << "Device is going down...";
                preDestruction(); // Give devices a chance to react
                emit("signalDeviceInstanceGone", m_devSrvInstId, m_instanceId);
                stopEventLoop();
                KARABO_LOG_INFO << "dead.";
            }

            karabo::util::Schema& getStateDependentSchema(const std::string& currentState) {
                boost::mutex::scoped_lock lock(m_stateDependendSchemaMutex);
                // Check cache, whether a special set of state-dependent expected parameters was created before
                std::map<std::string, karabo::util::Schema>::iterator it = m_stateDependendSchema.find(currentState);
                if (it == m_stateDependendSchema.end()) { // No
                    it = m_stateDependendSchema.insert(make_pair(currentState, Device::getSchema(m_classId, karabo::util::Schema::AssemblyRules(karabo::util::WRITE, currentState)))).first; // New one
                    if (!m_injectedSchema.empty()) it->second.merge(m_injectedSchema);
                }
                return it->second;
            }

            /**
             * Replace existing schema descriptions by static (hard coded in expectedParameters) part and
             * add additional (dynamic) descriptions
             * @param schema
             */
            void injectSchema(const karabo::util::Schema& schema) {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                m_stateDependendSchema.clear();
                m_injectedSchema = schema;
                m_fullSchema.merge(m_injectedSchema);
            }




        };
    }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::core::Device, TEMPLATE_CORE, DECLSPEC_CORE)

#endif
