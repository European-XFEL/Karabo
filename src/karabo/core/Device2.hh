/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_DEVICE2_HH
#define	KARABO_CORE_DEVICE2_HH

#include <log4cpp/Category.hh>

#include <karabo/util/Factory.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/log/Logger.hh>
#include "coredll.hh"

#include "FsmMacros.hh"
#include "SignalSlotable.hh"

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
         * The Device2 class.
         */
        template <class FSM >
        class Device2 : public karabo::xms::SignalSlotable, FSM {
        public:

            KARABO_CLASSINFO(Device2, "Device2", "1.0")
            KARABO_FACTORY_BASE_CLASS

            template <class Derived>
            Device2(Derived*) : m_log(0) {
                // Prepare expected parameters for runtime validation
                m_expectedInitialParameters = Device2::initialParameters(Derived::classInfo().getClassId());
                m_expectedReconfigurableParameters = Device2::reconfigurableParameters(Derived::classInfo().getClassId());
                m_expectedMonitoredParameters = Device2::monitorableParameters(Derived::classInfo().getClassId());
                m_allExpectedParameters = Device2::expectedParameters(Derived::classInfo().getClassId(), karabo::util::READ | karabo::util::WRITE | karabo::util::INIT);
            }

            virtual ~Device2() {
            }

            static void expectedParameters(karabo::util::Schema& expected) {

                using namespace karabo::util;
                using namespace karabo::net;

                CHOICE_ELEMENT<BrokerConnection > (expected).key("connection")
                        .displayedName("Connection")
                        .description("The connection to the communication layer of the distributed system")
                        .assignmentOptional().defaultValue("Jms")
                        .advanced()
                        .init()
                        .commit();

                STRING_ELEMENT(expected).key("devSrvInstId")
                        .displayedName("Device-Server Instance Id")
                        .description("The device-server instance id, on which this device-instance is running on")
                        .assignmentInternal().defaultValue("")
                        .commit();

                STRING_ELEMENT(expected).key("devInstId")
                        .displayedName("Device Instance Id")
                        .description("Device Instance Id uniquely identifies a device instance in the distributed system")
                        .assignmentOptional().noDefaultValue()
                        .init()
                        .advanced()
                        .commit();

                STRING_ELEMENT(expected).key("devClaId")
                        .displayedName("Device Class Id")
                        .description("The (factory)-name of the class of this device")
                        .readOnly()
                        .commit();

                STRING_ELEMENT(expected).key("state")
                        .displayedName("State")
                        .description("The current state the device is in")
                        .assignmentOptional().defaultValue("uninitialized")
                        .readOnly()
                        .commit();
            }

            void configure(const karabo::util::Hash& input) {

                using namespace std;
                using namespace karabo::util;
                using namespace karabo::io;
                using namespace karabo::net;

                try {

                    // Speed access to own classId
                    m_classId = getClassInfo().getClassId();

                    // Speed access to device-server instance
                    if (input.has("devSrvInstId")) {
                        input.get("devSrvInstId", m_devSrvInstId);
                    } else {
                        m_devSrvInstId = "";
                    }

                    // Construct needed for splitting the parameters (validate function needs this)
                    Hash tmp(m_classId, input);

                    // Set device instance 
                    string devInstId;
                    if (input.has("devInstId")) {
                        input.get("devInstId", devInstId);
                    } else { // No devInstId given
                        //devInstId = generateDefaultDeviceInstanceId();
                        tmp.setFromPath(m_classId + ".devInstId", devInstId);
                    }

                    // Setup logger
                    m_log = &(karabo::log::Logger::logger(devInstId));

                    // Split the configuration parameters into three pots
                    m_initialParameters = m_expectedInitialParameters.validate(tmp, true, false, true).get<Hash > (m_classId);
                    m_reconfigurableParameters = m_expectedReconfigurableParameters.validate(tmp, true, false, true).get<Hash > (m_classId);
                    m_monitoredParameters = m_expectedMonitoredParameters.validate(tmp, true, false, true).get<Hash > (m_classId);

                    // Instantiate connection
                    BrokerConnection::Pointer connection = BrokerConnection::createChoice("connection", input);

                    // Initialize the SignalSlotable instance
                    this->init(connection, devInstId);

                    SIGNAL4("signalErrorFound", string, string, string, string); // timeStamp, shortMsg, longMsg, instanceId
                    SIGNAL2("signalBadReconfiguration", string, string); // shortMsg, instanceId 
                    SIGNAL2("signalNoTransition", string, string);
                    SIGNAL3("signalChanged", karabo::util::Hash, string, string); // changeHash, instanceId, classId
                    SIGNAL4("signalWarning", string, string, string, string); // timeStamp, warnMsg, instanceId, priority
                    SIGNAL4("signalAlarm", string, string, string, string); // timeStamp, alarmMsg, instanceId, priority
                    SIGNAL3("signalSchemaUpdated", string, string, string); // schema, instanceId, classId
                    SIGNAL2("signalDeviceInstanceGone", string, string) /* DeviceServerInstanceId, deviceInstanceId */

                    SLOT1(slotReconfigure, karabo::util::Hash)
                    SLOT0(slotRefresh)
                    SLOT1(slotGetSchema, bool); // true = onlyCurrentState, false = fullSchema
                    SLOT0(slotKillDeviceInstance)

                            // Hard-coded connects (for global slots with this name)
                            this->connectN("", "signalChanged", "*", "slotChanged");
                    this->connectN("", "signalBadReconfiguration", "*", "slotBadReconfiguration");
                    this->connectN("", "signalNoTransition", "*", "slotNoTransition");
                    this->connectN("", "signalErrorFound", "*", "slotErrorFound");
                    this->connectN("", "signalWarning", "*", "slotWarning");
                    this->connectN("", "signalAlarm", "*", "slotAlarm");
                    this->connectN("", "signalSchemaUpdated", "*", "slotSchemaUpdated");
                    this->connectN("", "signalDeviceInstanceGone", "*", "slotDeviceInstanceGone");

                    KARABO_LOG_INFO << "Starting up " << m_classId << " on networkId " << getInstanceId();

                    if (m_devSrvInstId == boost::asio::ip::host_name()) {
                        std::stringstream stream;
                        Hash config("Xsd.indentation", -1);
                        Format<Schema>::create(config)->convert(m_allExpectedParameters, stream);
                        call("*", "slotNewStandaloneDeviceInstanceAvailable", boost::asio::ip::host_name(), tmp, getInstanceId(), stream.str());
                    }

                    set("devClaId", m_classId);


                } catch (const Exception& e) {
                    RETHROW;
                }
            }

            /**
             * Updates the state of the device. This function automatically notifies any observers.
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @param value The corresponding value (of corresponding data-type)
             */
            template <class T>
            void set(const std::string& key, const T& value, const karabo::util::Timestamp& timestamp = karabo::util::Timestamp()) {

                // NOTE We will have attributes in our next-generation Hash
                karabo::util::Hash h(key, value/*, karabo::util::Attributes("trainId", timestamp.getTrainId(), "msSinceEpoch", timestamp.getMsSinceEpoch())*/);
                this->set(h);
            }

            /**
             * Updates the state of the device with all key/value pairs given in the hash
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
             */
            void set(const karabo::util::Hash& hash) {

                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);

                // Flatten for safety
                karabo::util::Hash flat = hash.flatten();
                karabo::util::Hash::iterator it = flat.begin();
                while (it != flat.end()) {
                    const std::string& key = it->first;
                    if (m_expectedMonitoredParameters.hasKey(key)) {
                        m_monitoredParameters.setFromPath(key, it->second);
                    } else if (m_expectedReconfigurableParameters.hasKey(key)) {
                        m_reconfigurableParameters.setFromPath(key, it->second);
                    } else if (m_expectedInitialParameters.hasKey(key)) {
                        m_initialParameters.setFromPath(key, it->second);
                    } else {
                        // If we would follow J. Klora and the TANGO philosophy, the new attribute would just be injected into the system...
                        // We do not allow simple injections, yet
                        log() << log4cpp::Priority::WARN << "Illegal trial to set parameter (" + key + ") which was not described in the expectedParameters section";
                        flat.erase(it++);
                        continue;
                    }
                    ++it;
                }
                if (!flat.empty()) {
                    emit("signalChanged", flat.unflatten(), this->getInstanceId(), m_classId);
                }
            }

            /**
             * Retrieves the current value of any device2 parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device2 (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            T get(const std::string& key) {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                T t;
                if (m_monitoredParameters.tryToGetFromPath(key, t)) {
                    return t;
                } else if (m_reconfigurableParameters.tryToGetFromPath(key, t)) {
                    return t;
                } else if (m_initialParameters.tryToGetFromPath(key, t)) {
                    return t;
                } else {
                    throw PARAMETER_EXCEPTION("Illegal trial to get parameter (" + key + ") which was not described in the expectedParameters section");
                }
            }

            /**
             * Checks the type of any device2 parameter (that was defined in the expectedParameters function)
             * TODO: This does not support complex/nested types yet.
             * @param key A valid parameter of the device2 (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            bool is(const std::string& key) {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                if (m_monitoredParameters.has(key)) {
                    return m_monitoredParameters.is<T > (key);
                } else if (m_reconfigurableParameters.has(key)) {
                    return m_reconfigurableParameters.is<T > (key);
                } else if (m_initialParameters.has(key)) {
                    return m_initialParameters.is<T > (key);
                } else {
                    throw PARAMETER_EXCEPTION("Illegal trial to get parameter (" + key + ") which was not described in the expectedParameters section");
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
             * Retrieves all expected parameters of this device2
             * @return Schema object containing all expected parameters
             */
            karabo::util::Schema getFullSchema() const {
                if (!m_injectedExpectedParameters.empty())
                    return Schema(m_allExpectedParameters).addExternalSchema(m_injectedExpectedParameters);
                else return m_allExpectedParameters;
            }

            /**
             * Add external schema descriptions to current schema containers
             * @param schema
             */
            void appendSchema(const karabo::util::Schema& schema) {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                m_stateDependendSchema.clear();
                m_injectedExpectedParameters.addExternalSchema(schema);
            }

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
                karabo::util::Hash config("Xsd.indentation", -1);
                std::stringstream stream;
                karabo::io::Format<karabo::util::Schema>::create(config)->convert(getFullSchema(), stream);
                std::cout << "Serialized..." << std::endl;
                emit("signalSchemaUpdated", stream.str(), this->getInstanceId(), m_classId);
                log() << log4cpp::Priority::INFO << "Schema updated";
                // emit("signalSchemaUpdated", schema, 
            }

            /**
             * Converts a device2 parameter key into its aliased key (must be defined in the expectedParameters function)
             * @param key A valid parameter of the device2 (must be defined in the expectedParameters function)
             * @return Aliased representation of the parameter
             */
            template <class T>
            const T& key2alias(const std::string& key) {
                if (m_allExpectedParameters.hasKey(key)) {
                    return m_allExpectedParameters.key2alias<T > (key);
                } else {
                    throw PARAMETER_EXCEPTION("Illegal trial to get parameter (" + key + ") which was not described in the expectedParameters section");
                }
            }

            /**
             * Converts a device2 parameter alias into the original key (must be defined in the expectedParameters function)
             * @param key A valid parameter-alias of the device2 (must be defined in the expectedParameters function)
             * @return The original name of the parameter
             */
            template <class T>
            const std::string& alias2key(const T& alias) {
                if (m_allExpectedParameters.hasAlias(alias)) {
                    return m_allExpectedParameters.alias2key(alias);
                } else {
                    throw PARAMETER_EXCEPTION("The provided alias (" + karabo::util::String::toString(alias) + ") was not described in the expectedParameters section");
                }
            }

            /**
             * Checks if the argument is a valid alias of some key, i.e. defined in the expectedParameters function
             * @param alias Arbitrary argument of arbitrary type
             * @return true if it is an alias found in one of three containers of parameters:
             * "reconfigurable", "initial" or "monitored",  otherwise false
             */
            template <class T>
            const bool hasAlias(const T& alias) {
                return m_allExpectedParameters.hasAlias(alias);
            }

            /**
             * Checks if some alias is defined for the given key
             * @param key in expectedParameters mapping
             * @return true if the alias exists
             */
            bool keyHasAlias(const std::string& key) {
                return m_allExpectedParameters.keyHasAlias(key);
            }

            template <class T>
            bool aliasIsOfType(const std::string& key) {
                if (m_allExpectedParameters.hasKey(key)) {
                    return m_allExpectedParameters.aliasIsOfType<T > (key);
                }
                return false;
            }

            /**
             * Checks the type of any device2 parameter (that was defined in the expectedParameters function)
             * TODO: This does not support complex/nested types yet.
             * @param key A valid parameter of the device2 (must be defined in the expectedParameters function)
             * @return True if the stored and requested type fit, false otherwise
             */
            template <class T>
            bool parameterIsOfType(const std::string& key) {
                return m_allExpectedParameters.parameterIsOfType<T > (key);
            }

            karabo::util::Hash getInitialParameters() const {
                return m_initialParameters.flatten();
            }

            karabo::util::Hash getReconfigurableParameters() const {
                return m_reconfigurableParameters.flatten();
            }

            karabo::util::Hash getMonitorableParameters() const {
                return m_monitoredParameters.flatten();
            }

            karabo::util::Hash getCurrentConfiguration() const {
                using namespace karabo::util;
                Hash ret;
                Hash& config = ret.bindReference<Hash > (m_classId);
                config.update(m_initialParameters);
                config.update(m_reconfigurableParameters);
                config.update(m_monitoredParameters);
                return ret;
            }

            const std::string& getDeviceServerInstanceId() const {
                return m_devSrvInstId;
            }

            /**
             * This function will typically be called by the DeviceServer (or directly within the startDevice application).
             * It must be overriden in any derived device class and typically should block the main thread - bringing
             * the device into an event driven mode of operation.
             */
            virtual void run() = 0;

        protected: // Functions and Classes

            virtual void onReconfigure(karabo::util::Hash& incomingReconfiguration) {
            }

            virtual void onKill() {
            }

            void triggerErrorFound(const std::string& shortMessage, const std::string& detailedMessage) const {
                emit("signalErrorFound", karabo::util::Time::getCurrentDateTime(), shortMessage, detailedMessage, getInstanceId());
            }

            void triggerWarning(const std::string& warningMessage, const std::string& priority) const {
                emit("signalWarning", karabo::util::Time::getCurrentDateTime(), warningMessage, getInstanceId(), priority);
            }

            void triggerAlarm(const std::string& alarmMessage, const std::string& priority) const {
                emit("signalAlarm", karabo::util::Time::getCurrentDateTime(), alarmMessage, getInstanceId(), priority);
            }

            /**************************************************************/
            /*                 Some FSM convenience                       */

            /**************************************************************/


            KARABO_FSM_ON_EXCEPTION(onException)

            virtual void onException(const std::string& userMessage, const std::string& detailedMessage) {
                std::cout << "ERROR: " << userMessage << std::endl;
            }

            KARABO_FSM_LOGGER(log, log4cpp::CategoryStream, log4cpp::Priority::DEBUG)

            KARABO_FSM_NO_TRANSITION_V_ACTION(noStateTransition)

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
                KARABO_LOG_DEBUG << msg.str();
                emit("signalNoTransition", msg.str(), getInstanceId());
            }

            KARABO_FSM_ON_CURRENT_STATE_CHANGE(updateCurrentState)

            virtual void updateCurrentState(const std::string& currentState) {
                set("state", currentState);
                // Reply new state to interested event initiators
                // reply(state); // I believe this is buggy TODO: Check !!
            }

            KARABO_FSM_V_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string)

            void errorFoundAction(const std::string& shortMessage, const std::string& detailedMessage) {
                triggerErrorFound(shortMessage, detailedMessage);
            }



        private: // Functions

            void slotRefresh() {

                using namespace karabo::util;

                Hash all(m_initialParameters);
                all.update(m_reconfigurableParameters);
                all.update(m_monitoredParameters);
                emit("signalChanged", all, m_instanceId, m_classId);
                reply(all);
            }

            void slotReconfigure(const karabo::util::Hash& reconfiguration) {

                using namespace karabo::util;

                if (newConfiguration.empty()) return;

                std::pair<bool, std::string> result = validate(newConfiguration);

                if (result.first == true) { // is a valid reconfiguration
                    // Automatically flatten - for user convenience
                    //m_incomingValidatedReconfiguration = m_incomingValidatedReconfiguration.flatten();

                    // Give device-implementer a chance to specifically react on reconfiguration event by polymorphically calling back
                    try {
                        onReconfigure(m_incomingValidatedReconfiguration);
                    } catch (const karabo::util::Exception& e) {
                        onException(e.userFriendlyMsg(), e.detailedMsg());
                        reply(false, e.userFriendlyMsg());
                        return;
                    }

                    // Merge reconfiguration with current state
                    applyReconfiguration(m_incomingValidatedReconfiguration);
                }
                reply(result.first, result.second);
            }

            void slotGetSchema(const bool& onlyCurrentState) {
                if (onlyCurrentState) {
                    const string& currentState = get<string > ("state");
                    reply(getStateDependentSchema(currentState));
                } else {
                    reply(m_allExpectedParameters);
                }
            }

            karabo::util::Schema& getStateDependentSchema(const std::string& currentState) {
                boost::mutex::scoped_lock lock(m_stateDependendSchemaMutex);
                // Check cache, whether a special set of state-dependent expected parameters was created before
                map<string, Schema>::iterator it = m_stateDependendSchema.find(currentState);
                if (it == m_stateDependendSchema.end()) { // No
                    it = m_stateDependendSchema.insert(make_pair(currentState, Device2::expectedParameters(m_classId, karabo::util::WRITE, currentState))).first; // New one
                    if (!m_injectedExpectedParameters.empty()) it->second.addExternalSchema(m_injectedExpectedParameters);
                }
                return it->second;
            }

            void slotKillDeviceInstance() {
                log() << Priority::INFO << "Device is going down...";
                onKill(); // Give devices a chance to react
                emit("signalDeviceInstanceGone", m_devSrvInstId, m_instanceId);
                stopEventLoop();
                log() << Priority::INFO << "dead.";
            }

            std::pair<bool, std::string> validate(const karabo::util::Hash& reconfiguration) {
                // Retrieve the current state of the device instance
                const string& currentState = get<string > ("state");
                Schema& whiteList = getStateDependentSchema(currentState);
                Hash config(m_classId, newConfiguration); // Validator needs classId as root item
                log() << Priority::DEBUG << "Incoming reconfiguration:\n" << newConfiguration;
                try {

                    config = whiteList.validate(config, false, false, false, true);

                } catch (const ParameterException& e) {
                    string errorText = e.userFriendlyMsg() + " in state: \"" + currentState + "\"";
                    log() << Priority::ERROR << errorText;
                    return make_pair(false, errorText);
                }
                m_incomingValidatedReconfiguration = config.get<Hash > (m_classId);
                log() << Priority::DEBUG << "Validated reconfiguration:\n" << m_incomingValidatedReconfiguration;
                return make_pair(true, "");
            }

            void applyReconfiguration(const karabo::util::Hash& reconfiguration) {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                m_reconfigurableParameters.update(m_incomingValidatedReconfiguration);
                log() << Priority::DEBUG << "After user interaction:\n" << user;
                emit("signalChanged", m_incomingValidatedReconfiguration, getInstanceId(), m_classId);
                m_incomingValidatedReconfiguration.clear(); // No pending reconfiguration anymore
                log() << Priority::DEBUG << "Current state:\n" << m_reconfigurableParameters;
            }

            /**
             * Replace existing schema descriptions by static (hard coded in expectedParameters) part and
             * add additional (dynamic) descriptions
             * @param schema
             */
            void injectSchema(const karabo::util::Schema& schema) {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                m_stateDependendSchema.clear();
                m_injectedExpectedParameters.clear();
                m_injectedExpectedParameters = schema;
            }

        private: // Members

            // Members
            karabo::xms::SignalSlotable m_signalSlotable;

            karabo::util::Hash m_initialParameters;
            karabo::util::Hash m_reconfigurableParameters;
            karabo::util::Hash m_monitoredParameters;

            karabo::util::Schema m_expectedInitialParameters;
            karabo::util::Schema m_expectedReconfigurableParameters;
            karabo::util::Schema m_expectedMonitoredParameters;
            karabo::util::Schema m_allExpectedParameters;
            karabo::util::Schema m_injectedExpectedParameters;

            std::string m_classId;
            std::string m_devSrvInstId;

            std::map<std::string, karabo::util::Schema> m_stateDependendSchema;
            boost::mutex m_stateDependendSchemaMutex;

            karabo::util::Hash m_incomingValidatedReconfiguration;
            boost::mutex m_objectStateChangeMutex;

            log4cpp::Category* m_log;

        };

    }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::core::Device2<>, TEMPLATE_CORE, DECLSPEC_CORE)

#endif
