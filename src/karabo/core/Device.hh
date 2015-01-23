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
#include <karabo/xip/CpuImage.hh>
#include <karabo/xip/RawImageData.hh>
#include "coredll.hh"

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

            mutable boost::mutex m_objectStateChangeMutex;
            
            // Regular expression for error detection in state word
            boost::regex m_errorRegex;

            // progressBar related
            int m_progressMin;
            int m_progressMax;

            krb_log4cpp::Category* m_log;

            karabo::util::Hash m_parameters;
            karabo::util::Schema m_staticSchema;
            karabo::util::Schema m_injectedSchema;
            karabo::util::Schema m_fullSchema;

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
                
                INT32_ELEMENT(expected).key("nThreads")
                        .displayedName("Number of threads")
                        .description("Defines the number of threads that can be used to work on incoming events")
                        .assignmentOptional().defaultValue(1)
                        .minInc(1)
                        .adminAccess()
                        .commit();

                STRING_ELEMENT(expected).key("state")
                        .displayedName("State")
                        .description("The current state the device is in")
                        .readOnly().initialValue("Ok")
                        .commit();
                
                BOOL_ELEMENT(expected).key("trafficJam")
                        .displayedName("Traffic jam for messages")
                        .description("Flag denoting traffic jam for messages traveling via broker")
                        .readOnly().initialValue(false)
                        .commit();
                
                INT64_ELEMENT(expected).key("brokerLatency")
                        .displayedName("Broker latency (ms)")
                        .description("Time interval (in millis) between message sending to broker and receiving it on the device before queuing.")
                        .expertAccess()
                        .readOnly().initialValue(0)
                        //.warnHigh(10000LL)
                        .commit();
                
                INT64_ELEMENT(expected).key("processingLatency")
                        .displayedName("Processing latency (ms)")
                        .description("Time interval (in millis) between message sending to broker and reading it from the queue on the device.")
                        .readOnly().initialValue(0)
                        //.warnHigh(10000LL)
                        .commit();
                
                INT64_ELEMENT(expected).key("latencyUpper")
                        .displayedName("Latency upper limit")
                        .description("Message latency above that the \"Traffic jam\" flag will be set.")
                        .assignmentOptional().defaultValue(10000LL)
                        .adminAccess()
                        .commit();
                        
                INT64_ELEMENT(expected).key("latencyLower")
                        .displayedName("Latency lower limit")
                        .description("Message latency below that the \"Traffic jam\" flag will be unset.")
                        .assignmentOptional().defaultValue(5000LL)
                        .adminAccess()
                        .commit();
                        
                FSM::expectedParameters(expected);
            }

            Device(const karabo::util::Hash& configuration) : m_errorRegex(".*error.*", boost::regex::icase) {

                // Make the configuration the initial state of the device
                m_parameters = configuration;
                
                m_itself = this;     // switch on the updates of latencies using "updateLatencies()" of this class

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
                m_log = &(karabo::log::Logger::getLogger(m_deviceId)); // TODO use later: "device." + instanceId

                // Instantiate connection
                karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::createChoice("_connection_", configuration);

                // Initialize the SignalSlotable instance
                init(m_deviceId, connection);

                // Initialize FSM slots (the interface of this function must be inherited from the templated FSM)
                this->initFsmSlots(); // requires template CONCEPT

                // Initialize Device slots
                this->initDeviceSlots();
                
                // Register guard for slot calls
                this->registerSlotCallGuardHandler(boost::bind(&karabo::core::Device<FSM>::slotCallGuard, this, _1));
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
            void set(const std::string& key, const ValueType& value, const karabo::util::Timestamp& timestamp = karabo::util::Timestamp()) {
                karabo::util::Hash h(key, value);
                this->set(h, timestamp);
            }

            template <class PixelType>
            void set(const std::string& key, const karabo::xip::CpuImage<PixelType>& image, const karabo::util::Timestamp& timestamp = karabo::util::Timestamp()) {
                using namespace karabo::util;

                Dims dims(image.dimX(), image.dimY(), image.dimZ());
                karabo::xip::RawImageData raw(image.pixelPointer(), image.size(), true, dims);

                Hash hash(key, raw.hash());
                hash.setAttribute(key, "image", 1);
                m_parameters.merge(hash, karabo::util::Hash::REPLACE_ATTRIBUTES);
                emit("signalChanged", hash, getInstanceId());
            }

            void set(const std::string& key, const karabo::xip::RawImageData& image, const karabo::util::Timestamp& timestamp = karabo::util::Timestamp()) {
                using namespace karabo::util;

                Hash hash(key, image.hash());

                hash.setAttribute(key, "image", 1);
                m_parameters.merge(hash, karabo::util::Hash::REPLACE_ATTRIBUTES);
                emit("signalChanged", hash, getInstanceId());
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

            template <class ValueType>
            void setNoValidate(const std::string& key, const ValueType& value, const karabo::util::Timestamp& timestamp = karabo::util::Timestamp()) {
                karabo::util::Hash h(key, value);
                this->setNoValidate(h, timestamp);
            }

            void setNoValidate(const karabo::util::Hash& hash, const karabo::util::Timestamp& timestamp = karabo::util::Timestamp()) {
                // TODO Care about timestamps!!
                if (!hash.empty()) {
                    m_parameters.merge(hash, karabo::util::Hash::REPLACE_ATTRIBUTES);
                    emit("signalChanged", hash, getInstanceId());
                }
            }

            /**
             * Retrieves the current value of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            const T& get(const std::string& key, const T& var = T()) const {
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
            const karabo::util::Schema& getFullSchema() const {
                return m_fullSchema;
            }

            void appendSchema(const karabo::util::Schema& schema) {
                KARABO_LOG_DEBUG << "Append Schema requested";
                karabo::util::Hash validated;
                karabo::util::Validator::ValidationRules rules;
                rules.allowAdditionalKeys = true;
                rules.allowMissingKeys = true;
                rules.allowUnrootedConfiguration = true;
                rules.injectDefaults = true;
                rules.injectTimestamps = true;
                karabo::util::Validator v(rules);
                v.validate(schema, karabo::util::Hash(), validated, karabo::util::Timestamp());
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
                set(validated);

                KARABO_LOG_INFO << "Schema updated";
            }

            /**
             * Replace existing schema descriptions by static (hard coded in expectedParameters) part and
             * add additional (dynamic) descriptions
             * @param schema
             */
            void updateSchema(const karabo::util::Schema& schema) {

                KARABO_LOG_DEBUG << "Update Schema requested";
                karabo::util::Hash validated;
                karabo::util::Validator::ValidationRules rules;
                rules.allowAdditionalKeys = true;
                rules.allowMissingKeys = true;
                rules.allowUnrootedConfiguration = true;
                rules.injectDefaults = true;
                rules.injectTimestamps = true;
                karabo::util::Validator v(rules);
                v.validate(schema, karabo::util::Hash(), validated, karabo::util::Timestamp());
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

                KARABO_LOG_INFO << "Schema updated";

                // Notify the distributed system
                emit("signalSchemaUpdated", m_fullSchema, m_deviceId);

                // Merge all parameters
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
            const AliasType& getAliasFromKey(const std::string& key) const {
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
            const std::string& getKeyFromAlias(const AliasType& alias) const {
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
            const bool aliasHasKey(const T& alias) const {
                return m_fullSchema.aliasHasKey(alias);
            }

            /**
             * Checks if some alias is defined for the given key
             * @param key in expectedParameters mapping
             * @return true if the alias exists
             */
            bool keyHasAlias(const std::string& key) const {
                return m_fullSchema.keyHasAlias(key);
            }

            /**
             * Checks the type of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return The enumerated internal reference type of the value
             */
            karabo::util::Types::ReferenceType getValueType(const std::string& key) const {
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
                karabo::util::HashFilter::byTag(m_fullSchema, hash, filtered, tags);
                return filtered;
            }

            const std::string& getServerId() const {
                return m_serverId;
            }

            // This function will polymorphically be called by the FSM template

            virtual void updateState(const std::string& currentState) { // private
                KARABO_LOG_FRAMEWORK_DEBUG << "onStateUpdate: " << currentState;
                if (get<std::string>("state") != currentState) {
                    set("state", currentState);                    
                    if (boost::regex_match(currentState, m_errorRegex)) {
                        updateInstanceInfo(karabo::util::Hash("status", "error"));
                    } else {
                        // Reset the error status
                        if (getInstanceInfo().get<std::string>("status") == "error") {
                            updateInstanceInfo(karabo::util::Hash("status", "ok"));
                        }
                    }
                }
                // Reply new state to interested event initiators
                reply(currentState);
            }

            KARABO_DEPRECATED virtual void onStateUpdate(const std::string& currentState) {
                this->updateState(currentState);
            }

            /**
             * You can override this function to handle default caught exceptions differently
             * @param shortMessage short error message
             * @param detailedMessage detailed error message
             */
            void exceptionFound(const std::string& shortMessage, const std::string& detailedMessage) const {
                KARABO_LOG_ERROR << shortMessage;
                emit("signalNotification", std::string("EXCEPTION"), shortMessage, detailedMessage, m_deviceId);
            }

            //void notify("ERROR", const std::string& shortMessage, const std::string& detailedMessage)

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

            // Use execute instead to trigger your error event

            KARABO_DEPRECATED virtual void triggerError(const std::string& shortMessage, const std::string& detailedMessage) const {
                this->exceptionFound(shortMessage, detailedMessage);
            }

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
             * The call to run is blocking and afterwards communication should happen only via call-back hooks of the state-machine
             */
            void run() {

                // This initializations or done here and not in the constructor as they involve virtual function calls
                initClassId();
                initSchema();

                // Prepare some info further describing this particular instance
                // status, visibility, owner, lang 
                karabo::util::Hash instanceInfo;
                instanceInfo.set("type", "device");
                instanceInfo.set("classId", m_classId);
                instanceInfo.set("serverId", m_serverId);
                instanceInfo.set("visibility", this->get<int >("visibility"));
                instanceInfo.set("compatibility", Device::classInfo().getVersion());
                instanceInfo.set("host", boost::asio::ip::host_name());
                instanceInfo.set("status", "ok");
                instanceInfo.set("archive", this->get<bool>("archive"));
                
                boost::thread t(boost::bind(&karabo::core::Device<FSM>::runEventLoop, this, this->get<int>("heartbeatInterval"), instanceInfo, this->get<int>("nThreads")));

                // Give the broker communication some time to come up
                //boost::this_thread::sleep(boost::posix_time::milliseconds(100));

                KARABO_LOG_INFO << m_classId << " with deviceId: \"" << this->getInstanceId() << "\" got started";

                // ClassId
                m_parameters.set("classId", m_classId);
                // DeviceId
                m_parameters.set("deviceId", m_deviceId);
                // ServerId
                m_parameters.set("serverId", m_serverId);

                // Validate first time to assign timestamps
                m_objectStateChangeMutex.lock();

                // The following lines of code are needed to initially inject timestamps to the parameters
                karabo::util::Hash validated;
                std::pair<bool, std::string> result = m_validatorIntern.validate(m_fullSchema, m_parameters, validated, karabo::util::Timestamp());
                if (result.first == false) KARABO_LOG_WARN << "Bad parameter setting attempted, validation reports: " << result.second;
                m_parameters.merge(validated, karabo::util::Hash::REPLACE_ATTRIBUTES);
                m_objectStateChangeMutex.unlock();

                // Start the state machine
                this->startFsm(); // This function must be inherited from the templated base class (it's a concept!)

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

                SIGNAL2("signalChanged", karabo::util::Hash /*configuration*/, string /*deviceId*/);

                SIGNAL2("signalNoTransition", string, string);
                connect("", "signalNoTransition", "*", "slotNoTransition", NO_TRACK);

                SIGNAL4("signalNotification", string /*type*/, string /*messageShort*/, string /*messageDetail*/, string /*deviceId*/);
                connect("", "signalNotification", "*", "slotNotification", NO_TRACK);

                SIGNAL2("signalSchemaUpdated", karabo::util::Schema /*deviceSchema*/, string /*deviceId*/);
                connect("", "signalSchemaUpdated", "*", "slotSchemaUpdated", NO_TRACK);

                SLOT1(slotReconfigure, karabo::util::Hash /*reconfiguration*/)
                SLOT0(slotGetConfiguration)
                SLOT1(slotGetSchema, bool /*onlyCurrentState*/);
                SLOT0(slotKillDevice)
            }
            
            bool slotCallGuard(const std::string& slotName) {
                if (m_fullSchema.has(slotName)) {
                    const std::vector<std::string>& allowedStates = m_fullSchema.getAllowedStates(slotName);
                    if (!allowedStates.empty()) {
                        //std::cout << "Validating slot" << std::endl;
                        const std::string& currentState = get<std::string > ("state");                        
                        return std::find(allowedStates.begin(), allowedStates.end(), currentState) != allowedStates.end();
                    }
                    return true;
                }
                return true;                
            }

            void slotGetConfiguration() {
                //std::string senderId = this->getSenderInfo("slotGetConfiguration")->getInstanceIdOfSender();
                //call(senderId, "slotChanged", m_parameters, m_deviceId);
                reply(m_parameters, m_deviceId);
            }
            
             void slotGetSchema(bool onlyCurrentState) {

                //std::string senderId = this->getSenderInfo("slotGetSchema")->getInstanceIdOfSender();

                if (onlyCurrentState) {
                    const std::string& currentState = get<std::string > ("state");
                    const karabo::util::Schema& schema = getStateDependentSchema(currentState);
                    //call(senderId, "slotSchemaUpdated", schema, m_deviceId);
                    reply(schema, m_deviceId);
                } else {
                    //call(senderId, "slotSchemaUpdated", m_fullSchema, m_deviceId);
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

                        // Merge reconfiguration with current state
                        applyReconfiguration(validated);
                    }
                } catch (const karabo::util::Exception& e) {
                    this->exceptionFound(e.userFriendlyMsg(), e.detailedMsg());
                    reply(false, e.userFriendlyMsg());
                    return;
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
                this->stopWorkers();
                stopEventLoop();
            }

            karabo::util::Schema& getStateDependentSchema(const std::string& currentState) {
                KARABO_LOG_DEBUG << "call: getStateDependentSchema() for state: " << currentState;
                boost::mutex::scoped_lock lock(m_stateDependendSchemaMutex);
                // Check cache, whether a special set of state-dependent expected parameters was created before
                std::map<std::string, karabo::util::Schema>::iterator it = m_stateDependendSchema.find(currentState);
                if (it == m_stateDependendSchema.end()) { // No
                    it = m_stateDependendSchema.insert(make_pair(currentState, Device::getSchema(m_classId, karabo::util::Schema::AssemblyRules(karabo::util::WRITE, currentState)))).first; // New one
                    KARABO_LOG_DEBUG << "Providing freshly cached state-dependent schema:\n" << it->second;
                    if (!m_injectedSchema.empty()) it->second.merge(m_injectedSchema);
                } else {
                    KARABO_LOG_DEBUG << "Schema was already cached";
                }
                return it->second;
            }
            
            void updateLatencies() {
                bool jamFlag = this->get<bool>("trafficJam");
                long long latencyUpper = this->get<long long>("latencyUpper");
                long long latencyLower = this->get<long long>("latencyLower");
                
                karabo::util::Hash h("brokerLatency", m_brokerLatency, "processingLatency", m_processingLatency);

                if (jamFlag) {
                    if (m_processingLatency < latencyLower)
                        h.set("trafficJam", false);
                } else {
                    if (m_processingLatency > latencyUpper) {
                        h.set("trafficJam", true);
                        KARABO_LOG_WARN << "Processing latency " << m_processingLatency << " are higher than established limit : " << latencyUpper;
                    }
                }
                this->set(h, karabo::util::Timestamp());
            }
        };
    }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::core::Device, TEMPLATE_CORE, DECLSPEC_CORE)

#endif
