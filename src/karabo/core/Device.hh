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
#include <karabo/util.hpp>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/log/Logger.hh>
#include "coredll.hh"

#include "FsmMacros.hh"
#include "FsmBase.hh"
#include "DeviceClient.hh"

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
        
        #define KARABO_NO_SERVER "__none__"

        class BaseDevice : public virtual karabo::xms::SignalSlotable {

        public:

            KARABO_CLASSINFO(BaseDevice, "BaseDevice", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS;

            virtual void run() = 0;

            // TODO
            // Can be removed, if sending current configuration after instantiation by server is deprecated
            virtual karabo::util::Hash getCurrentConfiguration(const std::string& tags = "") = 0;

        };

        /**
         * The Device class.
         */
        template <class FSM = BaseFsm>
        class Device : public BaseDevice, public FSM {

            karabo::util::Validator m_validatorIntern;
            karabo::util::Validator m_validatorExtern;

            boost::shared_ptr<DeviceClient> m_deviceClient;

            std::string m_classId;
            std::string m_serverId;
            std::string m_deviceId;
            
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

            KARABO_CLASSINFO(Device, "Device", "1.0")

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

                VECTOR_STRING_ELEMENT(expected).key("visibility")
                        .displayedName("Visibility")
                        .description("Configures who is allowed to see this device at all")
                        .assignmentOptional().defaultValueFromString("")
                        .advanced()
                        .reconfigurable()
                        .commit();

                STRING_ELEMENT(expected).key("classId")
                        .displayedName("ClassID")
                        .description("The (factory)-name of the class of this device")
                        .advanced()
                        .readOnly()
                        .initialValue(Device::classInfo().getClassId())
                        .commit();

                STRING_ELEMENT(expected).key("serverId")
                        .displayedName("ServerID")
                        .description("The device-server on which this device is running on")
                        .advanced()
                        .assignmentOptional().noDefaultValue()
                        .init()
                        .commit();

                STRING_ELEMENT(expected).key("deviceId")
                        .displayedName("DeviceID")
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

                // Make the configuration the initial state of the device
                m_parameters = configuration;

                // Set serverId
                if (configuration.has("serverId")) configuration.get("serverId", m_serverId);
                else m_serverId = KARABO_NO_SERVER;

                // Set instanceId
                if (configuration.has("deviceId")) configuration.get("deviceId", m_deviceId);
                else m_deviceId = "__none__"; // TODO generate uuid

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
                m_log = &(karabo::log::Logger::getLogger(m_deviceId)); // TODO use later: "device." + instanceId

                // Instantiate connection
                karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::createChoice("connection", configuration);

                // Initialize the SignalSlotable instance
                init(connection, m_deviceId);

                // Initialize FSM slots (the interface of this function must be inherited from the templated FSM)
                this->initFsmSlots(); // requires template CONCEPT

                // Initialize Device slots
                this->initDeviceSlots();
            }

            virtual ~Device() {
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
            void set(const std::string& key, const ValueType& value, const karabo::util::Timestamp2& timestamp = karabo::util::Timestamp2()) {
                karabo::util::Hash h(key, value);
                this->set(h, timestamp);
            }
            
            /**
             * Updates the state of the device with all key/value pairs given in the hash
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
             */
            void set(const karabo::util::Hash& hash, const karabo::util::Timestamp2& timestamp = karabo::util::Timestamp2()) {
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
                        const Hash& desc = it->getValue<Hash>();
                        KARABO_LOG_WARN << desc.get<string>("message");
                        emit("signalNotification", desc.get<string>("type"), desc.get<string>("message"), string(), m_deviceId);
                    }
                }

                if (!validated.empty()) {
                    m_parameters.merge(validated, karabo::util::Hash::REPLACE_ATTRIBUTES);
                    emit("signalChanged", validated, getInstanceId());
                }
            }

            /**
             * Retrieves the current value of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            const T& get(const std::string& key, const T& var = T()) {
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                try {
                    return m_parameters.get<T>(key);
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst retrieving parameter (" + key + ") from device"));
                }
                return var; // never reached. Keep it to make the compiler happy.
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

                KARABO_LOG_DEBUG << "Update Schema requested";
                injectSchema(schema);
                KARABO_LOG_DEBUG << "Injected...";
                // Notify the distributed system
                emit("signalSchemaUpdated", m_fullSchema, m_deviceId);
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

            /**
             * Retrieves the current configuration.
             * If no argument is given, all parameters (those described in the expected parameters section) are returned.
             * A subset of parameters can be retrieved by specifying one or more tags.
             * @param tags The tags the parameter must carry to be retrieved
             * @return A Hash containing the current value of the selected configuration
             */
            karabo::util::Hash getCurrentConfiguration(const std::string& tags = "") {
                if (tags.empty()) return m_parameters;
                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
                karabo::util::Hash filtered;
                karabo::util::HashFilter::byTag(m_fullSchema, m_parameters, filtered, tags);
                return filtered;
            }
            
            karabo::util::Hash filterByTags(const karabo::util::Hash& hash, const std::string& tags) const {
                karabo::util::Hash filtered;
                karabo::util::HashFilter::byTag(m_fullSchema, hash, filtered, tags);
                return filtered;
            }

            const std::string& getServerId() const {
                return m_serverId;
            }

            // This function will polymorphically be called by the FSM template
            virtual void onStateUpdate(const std::string& currentState) { // private
                KARABO_LOG_FRAMEWORK_DEBUG << "onStateUpdate: " << currentState;
                if (get<std::string>("state") != currentState) {
                    set("state", currentState);
                }
                // Reply new state to interested event initiators
                reply(currentState);
            }

            // This function will polymorphically be called by the FSM template
            virtual void onNoStateTransition(const std::string& typeId, int state) {
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

        protected: // Functions and Classes

            // This function will polymorphically be called by the FSM template
            virtual void errorFoundAction(const std::string& shortMessage, const std::string& detailedMessage) {
                KARABO_LOG_ERROR << shortMessage;
                emit("signalNotification", std::string("ERROR"), shortMessage, detailedMessage, m_deviceId);
            }

            virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration) {
            }

            virtual void postReconfigure() {
            }

            virtual void preDestruction() {
            }

        private: // Functions

            /**
             * This function will typically be called by the DeviceServer (or directly within the startDevice application).
             * The call to run is blocking and afterwards communication should happen only via call-back hooks of the state-machine
             */
            void run() {

                // This initializations or done here and not in the constructor as they involve virtual function calls
                initClassId();
                initSchema();

                // Prepare some info further describing this particular instance
                // status, visibility, owner, lang 
                karabo::util::Hash info("type", "device", "classId", m_classId, "serverId", m_serverId, "visibility", this->get<std::vector<std::string> >("visibility"), "version", Device::classInfo().getVersion(), "host", boost::asio::ip::host_name());
                boost::thread t(boost::bind(&karabo::core::Device<FSM>::runEventLoop, this, true, info));
                this->startFsm();
                KARABO_LOG_INFO << m_classId << " with deviceId: \"" << this->getInstanceId() << "\" got started";

                // Repair classId
                m_parameters.set("classId", m_classId);

                // Validate first time to assign timestamps
                m_objectStateChangeMutex.lock();
                
                // The following lines of code are needed to initially inject timestamps to the parameters
                karabo::util::Hash validated;
                std::pair<bool, std::string> result = m_validatorIntern.validate(m_fullSchema, m_parameters, validated, karabo::util::Timestamp2());
                if (result.first == false) KARABO_LOG_WARN << "Bad parameter setting attempted, validation reports: " << result.second;
                m_parameters.merge(validated, karabo::util::Hash::REPLACE_ATTRIBUTES);
                m_objectStateChangeMutex.unlock();
                t.join(); // Blocks 
            }

            void initClassId() {
                m_classId = getClassInfo().getClassId();
            }

            void initSchema() {
                using namespace karabo::util;
                // The static schema is the regular schema as assembled by parsing the expectedParameters functions
                m_staticSchema = getSchema(m_classId, Schema::AssemblyRules(INIT | WRITE | READ));

                // At startup the static schema is identical with the runtime schema
                m_fullSchema = m_staticSchema;
            }

            void initDeviceSlots() {
                using namespace std;

                SIGNAL2("signalChanged", karabo::util::Hash, string); // changeHash, instanceId
                connectN("", "signalChanged", "*", "slotChanged");

                SIGNAL2("signalNoTransition", string, string);
                connectN("", "signalNoTransition", "*", "slotNoTransition");

                SIGNAL4("signalNotification", string, string, string, string); // type, messageShort, messageDetail, deviceId
                connectN("", "signalNotification", "*", "slotNotification");

                SIGNAL2("signalSchemaUpdated", karabo::util::Schema, string); // schema, deviceId
                connectN("", "signalSchemaUpdated", "*", "slotSchemaUpdated");

                SIGNAL3("signalProgressUpdated", int, string, string); // Progress value [0,100], label, deviceId
                connectN("", "progressUpdate", "*", "slotProgressUpdated");


                SLOT1(slotReconfigure, karabo::util::Hash /*reconfiguration*/)
                SLOT0(slotRefresh) // Deprecate
                SLOT0(slotGetConfiguration)
                SLOT1(slotGetSchema, bool /*onlyCurrentState*/);
                SLOT0(slotKillDevice)

            }

            // TODO deprecate
            void slotRefresh() {
                emit("signalChanged", m_parameters, m_deviceId);
                reply(m_parameters);
            }

            void slotGetConfiguration() {
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

                m_objectStateChangeMutex.lock();
                m_parameters.merge(reconfiguration);
                m_objectStateChangeMutex.unlock();

                KARABO_LOG_DEBUG << "After user interaction:\n" << reconfiguration;
                emit("signalChanged", reconfiguration, getInstanceId());
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

            void slotKillDevice() {
                // It is important to know who gave us the kill signal
                std::string senderId = getSenderInfo("slotKillDevice")->getInstanceIdOfSender();
                if (senderId == m_serverId) { // Our server killed us
                    KARABO_LOG_INFO << "Device is going down as instructed by server";
                    preDestruction(); // Give devices a chance to react
                    stopEventLoop();

                } else { // Someone else wants to see us dead, we should inform our server
                    KARABO_LOG_INFO << "Device is going down as instructed by \"" << senderId << "\"";
                    call(m_serverId, "slotDeviceGone", m_deviceId);
                    preDestruction(); // Give devices a chance to react
                    stopEventLoop();
                }
            }

            karabo::util::Schema& getStateDependentSchema(const std::string& currentState) {
                KARABO_LOG_DEBUG << "call: getStateDependentSchema()";
                boost::mutex::scoped_lock lock(m_stateDependendSchemaMutex);
                // Check cache, whether a special set of state-dependent expected parameters was created before
                std::map<std::string, karabo::util::Schema>::iterator it = m_stateDependendSchema.find(currentState);
                if (it == m_stateDependendSchema.end()) { // No
                    it = m_stateDependendSchema.insert(make_pair(currentState, Device::getSchema(m_classId, karabo::util::Schema::AssemblyRules(karabo::util::WRITE, currentState)))).first; // New one
                    KARABO_LOG_DEBUG << "Providing freshly cached state-dependent schema:\n" << it->second;
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
