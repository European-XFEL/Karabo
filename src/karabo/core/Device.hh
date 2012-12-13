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

#include <log4cpp/Category.hh>

#include <karabo/util/Factory.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/log/Logger.hh>
#include "coredll.hh"

#include "FsmMacros.hh"
#include "DeviceClient.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace core {
        
        // Convenient logging
#ifdef KARABO_ENABLE_TRACE_LOG
#define KARABO_LOG_TRACE if(0); else std::cerr
#else 
#define KARABO_LOG_TRACE if(1); else std::cerr
#endif
        
#define KARABO_LOG_DEBUG log() << log4cpp::Priority::DEBUG 
#define KARABO_LOG_INFO  log() << log4cpp::Priority::INFO 
#define KARABO_LOG_WARN  log() << log4cpp::Priority::WARN 
#define KARABO_LOG_ERROR log() << log4cpp::Priority::ERROR 

        /**
         * The Device class.
         */
        class Device : public karabo::xms::SignalSlotable {
        public:

            KARABO_CLASSINFO(Device, "Device", "1.0")
            KARABO_FACTORY_BASE_CLASS

            template <class Derived>
            Device(Derived*) : m_log(0), m_progressMin(0), m_progressMax(100) {
                // Prepare expected parameters for runtime validation
                m_expectedInitialParameters = Device::initialParameters(Derived::classInfo().getClassId());
                m_expectedReconfigurableParameters = Device::reconfigurableParameters(Derived::classInfo().getClassId());
                m_expectedMonitoredParameters = Device::monitorableParameters(Derived::classInfo().getClassId());
                m_allExpectedParameters = Device::expectedParameters(Derived::classInfo().getClassId(), karabo::util::READ | karabo::util::WRITE | karabo::util::INIT);
            }

            virtual ~Device();

            static void expectedParameters(karabo::util::Schema& expected);

            void configure(const karabo::util::Hash& input);
            
            /**
             * This function allows to communicate to other (remote) devices.
             * Any device contains also a controller for other devices (DeviceClient) 
             * which is returned by this function.
             * 
             * @return DeviceClient instance
             */
            DeviceClient& remote();
            

            /**
             * Updates the state of the device. This function automatically notifies any observers.
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @param value The corresponding value (of corresponding data-type)
             */
            template <class T>
            void set(const std::string& key, const T& value) {

                boost::mutex::scoped_lock lock(m_objectStateChangeMutex);

                if (m_expectedMonitoredParameters.hasKey(key)) {
                    m_monitoredParameters.setFromPath(key, value);
                } else if (m_expectedReconfigurableParameters.hasKey(key)) {
                    m_reconfigurableParameters.setFromPath(key, value);
                } else if (m_expectedInitialParameters.hasKey(key)) {
                    m_initialParameters.setFromPath(key, value);
                } else {
                    // If we would follow J. Klora and the TANGO philosophy, the new attribute would just be injected into the system...
                    // We do not allow any injections, yet
                    log() << log4cpp::Priority::WARN << "Illegal trial to set parameter (" + key + ") which was not described in the expectedParameters section";
                }
                // Automatically inform the distributed system
                karabo::util::Hash config(key, value);
                emit("signalChanged", config, getInstanceId(), m_classId);
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
                        // We do not allow any injections, yet
                        log() << log4cpp::Priority::WARN << "Illegal trial to set parameter (" + key + ") which was not described in the expectedParameters section";
                        flat.erase(it++);
                        continue;
                    }
                    ++it;
                }
                if (!flat.empty()) {
                    emit("signalChanged", flat.unflatten(), getInstanceId(), m_classId);
                }
            }

            /**
             * Retrieves the current value of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
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
             * Checks the type of any device parameter (that was defined in the expectedParameters function)
             * TODO: This does not support complex/nested types yet.
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
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
            log4cpp::Category& log();


            /**
             * Retrieves all expected parameters of this device
             * @return Schema object containing all expected parameters
             */
            karabo::util::Schema getFullSchema() const;

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
                emit("signalSchemaUpdated", stream.str(), getInstanceId(), m_classId);
                log() << log4cpp::Priority::INFO << "Schema updated";
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
            template <class T>
            const T& key2alias(const std::string& key) {
                if (m_allExpectedParameters.hasKey(key)) {
                    return m_allExpectedParameters.key2alias<T > (key);
                } else {
                    throw PARAMETER_EXCEPTION("Illegal trial to get parameter (" + key + ") which was not described in the expectedParameters section");
                }
            }

            /**
             * Converts a device parameter alias into the original key (must be defined in the expectedParameters function)
             * @param key A valid parameter-alias of the device (must be defined in the expectedParameters function)
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
             * Checks the type of any device parameter (that was defined in the expectedParameters function)
             * TODO: This does not support complex/nested types yet.
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return True if the stored and requested type fit, false otherwise
             */
            template <class T>
            bool parameterIsOfType(const std::string& key) {
                return m_allExpectedParameters.parameterIsOfType<T > (key);
            }


            karabo::util::Hash getInitialParameters() const;

            karabo::util::Hash getReconfigurableParameters() const;

            karabo::util::Hash getMonitorableParameters() const;

            karabo::util::Hash getCurrentConfiguration() const;

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

            virtual void onException(const std::string& userMessage, const std::string& detailedMessage) {
                std::cout << "ERROR: " << userMessage << std::endl;
            }

            virtual void onKill() {
            }

            void triggerErrorFound(const std::string& shortMessage, const std::string& detailedMessage) const;
            void triggerWarning(const std::string& warningMessage, const std::string& priority) const;
            void triggerAlarm(const std::string& alarmMessage, const std::string& priority) const;

            /**************************************************************/
            /*                 Some FSM convenience                       */
            /**************************************************************/

            KARABO_FSM_ON_EXCEPTION(onException)

            KARABO_FSM_LOGGER(log, log4cpp::CategoryStream, log4cpp::Priority::DEBUG)

            KARABO_FSM_NO_TRANSITION_V_ACTION(noStateTransition)

            KARABO_FSM_ON_CURRENT_STATE_CHANGE(updateCurrentState)

            KARABO_FSM_V_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string)

        protected: // Members

            karabo::util::Hash m_initialParameters;
            karabo::util::Hash m_reconfigurableParameters;
            karabo::util::Hash m_monitoredParameters;

            karabo::util::Schema m_expectedInitialParameters;
            karabo::util::Schema m_expectedReconfigurableParameters;
            karabo::util::Schema m_expectedMonitoredParameters;
            karabo::util::Schema m_allExpectedParameters;

            karabo::util::Schema m_injectedExpectedParameters;


        private: // Functions

            void slotRefresh();

            void slotReconfigure(const karabo::util::Hash& reconfiguration);

            void slotGetSchema(const bool& onlyCurrentState);

            void slotKillDeviceInstance();

            std::pair<bool, std::string> validate(const karabo::util::Hash& reconfiguration);

            karabo::util::Schema& getStateDependentSchema(const std::string& currentState);

            void applyReconfiguration(const karabo::util::Hash& reconfiguration);

            void increaseInstanceCount();

            void decreaseInstanceCount();

            std::string generateDefaultDeviceInstanceId();

            template <class T>
            void checkWarningsAndAlarms(const std::string& key, const T& value) {
                size_t pos = key.find_last_of(".");
                if (pos != std::string::npos && key.substr(pos) == ".actual") {
                    std::string baseKey = key.substr(0, key.find_last_of(".") + 1);
                    std::cout << "BaseKey: " << baseKey << std::endl;
                    T threshold;
                    if (m_reconfigurableParameters.tryToGetFromPath(baseKey + "warnLow", threshold)) {
                        if (value < threshold) {
                            std::ostringstream os;
                            os << " Value " << value << " of parameter \"" << key << "\" went below warn level of " << threshold;
                            log() << log4cpp::Priority::WARN << os.str();
                            triggerWarning(os.str(), std::string("MEDIUM"));
                        }
                    }
                    if (m_reconfigurableParameters.tryToGetFromPath(baseKey + "warnHigh", threshold)) {
                        if (value > threshold) {
                            std::ostringstream os;
                            os << " Value " << value << " of parameter \"" << key << "\" went above warn level of " << threshold;
                            log() << log4cpp::Priority::WARN << os.str();
                            triggerWarning(os.str(), std::string("MEDIUM"));
                        }
                    }
                    if (m_reconfigurableParameters.tryToGetFromPath(baseKey + "alarmLow", threshold)) {
                        if (value < threshold) {
                            std::ostringstream os;
                            os << " Value " << value << " of parameter \"" << key << "\" went below alarm level of " << threshold;
                            log() << log4cpp::Priority::WARN << os.str();
                            triggerAlarm(os.str(), std::string("HIGH"));
                        }
                    }
                    if (m_reconfigurableParameters.tryToGetFromPath(baseKey + "alarmHigh", threshold)) {
                        if (value > threshold) {
                            std::ostringstream os;
                            os << " Value " << value << " of parameter \"" << key << "\" went above alarm level of " << threshold;
                            log() << log4cpp::Priority::WARN << os.str();
                            triggerAlarm(os.str(), std::string("HIGH"));
                        }
                    }
                }
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
            
            boost::shared_ptr<DeviceClient> m_deviceClient;

            std::string m_classId;
            std::string m_devSrvInstId;
            
            std::map<std::string, karabo::util::Schema> m_stateDependendSchema;
            boost::mutex m_stateDependendSchemaMutex;

            karabo::util::Hash m_incomingValidatedReconfiguration;
            boost::mutex m_objectStateChangeMutex;

            log4cpp::Category* m_log;
            
            // progressBar related
            int m_progressMin;
            int m_progressMax;
        };
    }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::core::Device, TEMPLATE_CORE, DECLSPEC_CORE)

#endif
