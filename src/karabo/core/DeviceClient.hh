/*
 * $Id: Com.hh 6624 2012-06-27 12:57:09Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_DEVICE_CLIENT_HH
#define	KARABO_CORE_DEVICE_CLIENT_HH

#include <karabo/xms/SignalSlotable.hh>
#include <karabo/util/Timestamp.hh>
#include <karabo/util/Schema.hh>
#include <karabo/core/Lock.hh>

#include <map>
#include <set>
#include <string>

#define KARABO_GET_SHARED_FROM_WEAK(sp, wp) \
auto sp = wp.lock(); \
if (!sp) throw KARABO_LOGIC_EXCEPTION(std::string(#wp) + " object already deleted");

namespace karabo {

    namespace core {

        // Forward
        template <class T>
        class Device;

        /**
         * @class DeviceClient
         * @brief This class can be used to (remotely) control devices of the distributed system
         *        Synchronous calls (i.e. get()) are in fact asynchronous under the hood\
         *
         * The Karabo DeviceClient provides a high-level interface for common calls to (remote) devices
         * in the distributed system. In principle functionality implemented in the DeviceClient can
         * be fully implemented in the Device using low level SignalSlotable calls alone, but device
         * developers are discouraged from this approach, especially if synchronous behavior is
         * acceptable or even desired.
         *
         * In the context of a Device the DeviceClient is available using the Device::remote() function;
         * it then shares the SignalSlotable instance of the device, e.g. there is no instantiation overhead.
         */
        class DeviceClient : public boost::enable_shared_from_this<DeviceClient> {


            template<class T>
            friend class Device;

            typedef std::map<std::string, unsigned int> InstanceUsage;
            /// keys are instance IDs, values are a sets of properties that changed
            typedef std::map<std::string, std::set<std::string> > SignalChangedMap;
            typedef boost::function<void (const karabo::util::Hash& /*topologyEntry*/) > InstanceNewHandler;
            typedef boost::function<void (const karabo::util::Hash& /*topologyEntry*/) > InstanceUpdatedHandler;
            typedef boost::function<void (const std::string& /*instanceId*/, const karabo::util::Hash& /*instanceInfo*/) > InstanceGoneHandler;
            typedef boost::function<void (const std::string& /*deviceId*/, const karabo::util::Schema& /*schema*/) > SchemaUpdatedHandler;
            typedef boost::function<void (const std::string& /*serverId*/, const std::string& /*classId*/, const karabo::util::Schema& /*schema*/) > ClassSchemaHandler;

            static const unsigned int CONNECTION_KEEP_ALIVE = 15;

            boost::shared_ptr<karabo::xms::SignalSlotable> m_internalSignalSlotable;

        protected: // members

            /**
             * server +
             *   <serverId> type host version status deviceClasses +
             *     classes +
             *       <classId> +
             *         description SCHEMA
             *         configuration HASH
             *     description SCHEMA
             *     configuration HASH
             *
             * device +
             *   <deviceId> type host version status classId serverId +
             *      fullSchema => SCHEMA
             *      configuration => HASH
             *      activeSchema +
             *         <stateName> => SCHEMA
             *
             */
            karabo::util::Hash m_runtimeSystemDescription;

            mutable boost::mutex m_runtimeSystemDescriptionMutex;

            boost::weak_ptr<karabo::xms::SignalSlotable> m_signalSlotable;

            bool m_isShared;           

            InstanceUsage m_instanceUsage;

            karabo::util::Hash m_deviceChangedHandlers;

            karabo::util::Hash m_propertyChangedHandlers;

            boost::mutex m_instanceUsageMutex;

            boost::mutex m_deviceChangedHandlersMutex;

            boost::mutex m_propertyChangedHandlersMutex;

            int m_internalTimeout;

            bool m_topologyInitialized;

            boost::asio::deadline_timer m_ageingTimer;

            static const unsigned int m_ageingIntervallMilliSec;
            static const unsigned int m_ageingIntervallMilliSecCtr;

            bool m_getOlder; /// defines whether aging is running or not

            boost::thread m_signalsChangedThread;
            bool m_runSignalsChangedThread;
            boost::posix_time::milliseconds m_signalsChangedInterval;
            boost::mutex m_signalsChangedMutex;
            SignalChangedMap m_signalsChanged; /// map of collected signalChanged

            boost::mutex m_loggerMapMutex;

            util::Hash m_loggerMap;

            bool m_loggerMapCached;

            InstanceNewHandler m_instanceNewHandler;
            InstanceUpdatedHandler m_instanceUpdatedHandler;
            InstanceGoneHandler m_instanceGoneHandler;
            SchemaUpdatedHandler m_schemaUpdatedHandler;
            ClassSchemaHandler m_classSchemaHandler;

            std::set<std::string> m_immortals;
            mutable boost::mutex m_immortalsMutex;

            int m_accessLevel = karabo::util::Schema::ADMIN;

        public:

            KARABO_CLASSINFO(DeviceClient, "DeviceClient", "1.2");

            /**
             * Constructor which establishes an own connection to the communication system.
             * This constructor is intended for stand-alone C++ device clients. Once we care about authentication,
             * this has to be added here.
             * @param instanceId The id with which the client should participate in the system.
             *                   If not unique or invalid, constructor will throw an exception.
             *                   If empty (i.e. default), an id will be generated from host name and process id.
             */
            DeviceClient(const std::string& instanceId = std::string());

            /**
             * Constructor using instantiated signalSlotable class (shared communication)
             * @param signalSlotable An instance of the SignalSlotable lass
             */
            DeviceClient(const boost::shared_ptr<karabo::xms::SignalSlotable>& signalSlotable);

            /**
             * Destructor joins the underlying event thread,
             * i.e. execution takes some time (1-2 seconds).
             */
            virtual ~DeviceClient();

            bool login(const std::string& username, const std::string& password, const std::string& provider = "LOCAL");

            bool logout();

            /**
             * Sets the internal timeout for any request/response like communications
             * @param internalTimeout The default timeout in ms
             */
            void setInternalTimeout(const unsigned int internalTimeout);

            /**
             * Retrieves the currently set internal timeout
             * @return default timeout in ms
             */
            int getInternalTimeout() const;



            /**
             * Set ageing on or off (on by default)
             * @return
             */
            void setAgeing(bool toggle);

            /**
             * Set interval to wait between subsequent (for the same instance)
             * calls to handlers registered via registerDeviceMonitor.
             * Changes received within that interval will be cached and, in case of
             * several updates of the same property within the interval, only the most
             * up-to-date value will be handled.
             * If negative, switch off caching and call handler immediately.
             */
            void setDeviceMonitorInterval(long int milliseconds);

            /**
             * Allows asking whether an instance is online in the current distributed system
             * @param boolean indicating whether existing and hostname if exists
             * @return
             */
            std::pair<bool, std::string> exists(const std::string& instanceId);

            /**
             * Enables tracking of new and departing device instances
             *
             * The handlers registered with registerInstance[New|Gone|Updated]Monitor
             * will be called accordingly. If the handler for instanceNew is registered before
             * calling this method, it will be called for each device currently in the system.
             *
             * NOTE: Use wisely!
             * There is a performance cost to tracking all devices since it means
             * subscribing to the heartbeats of all servers and devices in the system.
             */
            void enableInstanceTracking();

            /**
             * Returns the full information about the current (runtime) distributed system
             * @return a Hash containing the full system description
             */
            karabo::util::Hash getSystemInformation();


            /**
             * Returns only the topology of the current system (no instance configurations or descriptions)
             * @return Hash containing the topology of the runtime system
             */
            karabo::util::Hash getSystemTopology();

            /**
             * Retrieves all servers currently existing in the distributed system
             * @return array of server ids
             */
            std::vector<std::string> getServers();

            /**
             * Retrieves all device classes (plugins) available on a given device server
             * @param deviceServer device server id
             * @return array of device classes
             */
            std::vector<std::string> getClasses(const std::string& deviceServer);

            /**
             * Retrieves all devices (instances) available on a given device server
             * @param deviceServer device server id
             * @return array of device instanceIds
             */
            std::vector<std::string> getDevices(const std::string& deviceServer);

            /**
             * Retrieves all devices in the distributed system.
             * @return array of device instanceIds
             */
            std::vector<std::string> getDevices();

            /**
             * Retrieves the full Schema (parameter description) of the given instance;
             * @param instanceId Device's instance ID
             * @return full Schema
             */
            karabo::util::Schema getDeviceSchema(const std::string& instanceId);


            /**
             * Retrieves the full Schema (parameter description) of the given instance
             * The call is non-blocking, if no Schema is currently available the return
             * will be empty. However, the schema request will be sent and should lead to
             * later arrival of a schema.
             * @param instanceId Device's instance ID
             * @return full Schema
             */
            karabo::util::Schema getDeviceSchemaNoWait(const std::string& instanceId);

            /**
             * Retrieves the currently active Schema (filtered by allowed states and allowed roles)
             * of the given instance
             * @param instanceId Device's instance ID
             * @return active Schema
             */
            karabo::util::Schema getActiveSchema(const std::string& instanceId);

            /**
             * Retrieves a schema from static context of a loaded Device class plug-in.
             * This schema represents a description of parameters possible to configure for instantiation.
             * I.e. returns in fact a description of the constructor arguments to that device class.
             * @param serverId instanceId of a deviceServer
             * @param classId name of loaded class on the deviceServer (classId)
             * @return Schema describing parameters available at instantiation time
             */
            karabo::util::Schema getClassSchema(const std::string& serverId, const std::string& classId);

            /**
             * Retrieves a schema from static context of a loaded Device class plug-in.
             * This schema represents a description of parameters possible to configure for instantiation.
             * This function can be used to pre-cache a schema for later usage. It returns an empty schema.
             * @param serverId instanceId of a deviceServer
             * @param classId name of loaded class on the deviceServer (classId)
             * @return an empty schem
             */
            karabo::util::Schema getClassSchemaNoWait(const std::string& serverId, const std::string& classId);

            /**
             * Retrieve the properties of a device at deviceId.
             * @param deviceId of the device to request information from
             * @return a vector containing the property paths of the device
             */
            std::vector<std::string> getProperties(const std::string& deviceId);

            /**
             * Retrieve the properties of a class loaded on a server
             * @param serverId server to request information from
             * @param classId of the class
             * @return  vector containing the property paths of the class
             */
            std::vector<std::string> getClassProperties(const std::string& serverId, const std::string& classId);

            /**
             * Retrieve a list of commands that may be currently executed on a
             * device in the distributed system. Available commands are determined
             * by device state and access rights.
             * @param instanceId of the device to ask for available commands
             * @return a vector containing the slot names of the commands that can be executed
             */
            std::vector<std::string> getCurrentlyExecutableCommands(const std::string& instanceId);

            /**
             * Retrieve a list of properties that may be currently altered on a
             * device in the distributed system. Available properties are determined
             * by device state and access rights.
             * @param instanceId of the device to ask for settable properties
             * @return a vector containing the slot names of the properties that can be altered.
             */
            std::vector<std::string> getCurrentlySettableProperties(const std::string& instanceId);

            /**
             * Load a device configuration from a file
             * @param filename
             * @return a Hash containing the configuration
             */
            karabo::util::Hash loadConfigurationFromFile(const std::string& filename);

            /**
             * Attempt to instantiate a device of the specified class, on a remote server with a given initial configuration
             * 
             * @param serverInstanceId of the server to instantiate the device on. Needs to have the device plugin available
             * @param classId of the device to be instantiate
             * @param configuration Hash which contains the initial device configuration. It must have one out of the two 
             *        following forms:
             *          option 1: 
             *               - key "classId" pointing to a string,
             *          option 2: 
             *               - no classId specified: class id to be instantiated is taken from classId parameter
             *          option 3 (for backward compatibility - not recommended):
             *               - a single key (e.g. "myClassId") representing the classId
             *               - the value for this key is a Hash with all the non-default properties
             * 
             * @param timeoutInSeconds by default set to -1, which means block indefinitely, if a positive value an Exception is thrown
             *        if the device hasn't been instantiated.
             * @return (ok, reply) pair where ok is true if no exception occurred and reply is the answer received from server
             */
            std::pair<bool, std::string> instantiate(const std::string& serverInstanceId, const std::string& classId,
                                                     const karabo::util::Hash& configuration = karabo::util::Hash(),
                                                     int timeoutInSeconds = -1);

            /**
             * Instantiate a device on a remote server
             * @param serverInstanceId of the server to instantiate the device on. Needs to have the device plugin available
             * @param configuration Hash which contains the initial device configuration. The 'classId' attribute must be present.
             * @param timeoutInSeconds by default set to -1, which means block indefinitely, if a positive value an Exception is thrown
             *        if the device hasn't been instantiated.
             * @return
             */
            std::pair<bool, std::string> instantiate(const std::string& serverInstanceId,
                                                     const karabo::util::Hash& configuration,
                                                     int timeoutInSeconds = -1);

            
            /**
             * Utility method that takes care of adding classId to configuration of device to be instantiated 
             * by instantiate and instantiateNoWait. If configuration does not have 'classId' key, this is added, 
             * with the value of classId parameter. Otherwise the configuration 'classId' value is used.
             * In the latter case, if the value of classId parameter mismatches the one of 'classId' attribute of configuration 
             * a warning is thrown.
             * @param classId of the device to be instantiated. 
             * @param configuration of the device to be instantiated.
             * @return configuration ready to be sent to device server
             */
            karabo::util::Hash formatConfigToInstantiate(const std::string& classId, const karabo::util::Hash& configuration);
            
            /**
             * Instantiate a device on a remote server. In contrast to DeviceClient::instantiate, this function returns
             * immediately.
             * 
             * @param serverInstanceId of the server to instantiate the device on. Needs to have the device plugin available
             * @param classId of the device to be instantiate
             * @param configuration Hash which contains the initial device configuration. It must have one out of the two 
             *        following forms:
             *          option 1: 
             *               - key "classId" pointing to a string,
             *          option 2: 
             *               - no classId specified: class id to be instantiated is taken from classId parameter
             *          option 3 (for backward compatibility - not recommended):
             *               - a single key (e.g. "myClassId") representing the classId
             *               - the value for this key is a Hash with all the non-default properties
             *
             */
            void instantiateNoWait(const std::string& serverInstanceId, const std::string& classId,
                                   const karabo::util::Hash& configuration = karabo::util::Hash());

            /**
             * Instantiate a device on a remote server. In contrast to DeviceClient::instantiate, this function returns
             * immediately.
             * @param serverInstanceId of the server to instantiate the device on. Needs to have the device plugin available
             * @param configuration Hash which contains the initial device configuration. The 'classId' attribute must be present.
             * @return
             */
            void instantiateNoWait(const std::string& serverInstanceId, const karabo::util::Hash& configuration);

            /**
             * Kill a device in the distributed system and wait until it is actually dead
             * @param deviceId of the device to kill
             * @param timeoutInSeconds timeoutInSeconds by default set to -1, which means block indefinitely, if a positive value an Exception is thrown
             *        if the device hasn't been killed.
             * @return
             */
            std::pair<bool, std::string> killDevice(const std::string& deviceId, int timeoutInSeconds = -1);

            /**
             * Kill a device in the distributed system and return immediately
             * @param deviceId of the device to kill
             * @return
             */
            void killDeviceNoWait(const std::string& deviceId);

            /**
             * Kill a device server in the distributed system and all its associated devices. Waits til the server
             * is dead.
             *
             * @param serverId of the server to kill
             * @param timeoutInSeconds timeoutInSeconds timeoutInSeconds by default set to -1, which means block indefinitely, if a positive value an Exception is thrown
             *        if the device server hasn't been killed.
             * @return
             */
            std::pair<bool, std::string> killServer(const std::string& serverId, int timeoutInSeconds = -1);

            /**
             * Kill a device server in the distributed system and all its associated devices. Returns immediately.
             *
             * @param serverId of the server to kill
             * @return
             */
            void killServerNoWait(const std::string& serverId);

            /**
             * Return the configuration Hash of an instance. The configuration
             * is internally cached, so it does not necessarily result in a query to the distributed system if
             * the device configuration has not changed since the last query.
             * @param instanceId for which to return the configuration of
             * @return a Hash holding the instance configuration
             */
            karabo::util::Hash get(const std::string& instanceId);

            /**
             * Return the configuration Hash of an instance. The configuration
             * is internally cached, so it does not necessarily result in a query to the distributed system if
             * the device configuration has not changed since the last query.
             *
             * @param instanceId for which to return the configuration of
             * @param hash reference to write configuration into
             */
            void get(const std::string& instanceId, karabo::util::Hash& hash);

            /**
             * Return the cached configuration if it is still valid, otherwise query an updated version
             * but return an empty Hash.
             * @param deviceId for which to return the configuration of
             * @return a Hash holding the instance configuration
             */
            karabo::util::Hash getConfigurationNoWait(const std::string& deviceId);

            /**
             * Check if an attribute exists for a property on a given instance
             * @param instanceId to check on
             * @param key path to the property to check if it has a given attribute
             * @param attribute to check for
             * @param keySep path separator
             * @return a boolean indicating if the attribute is present
             */
            bool hasAttribute(const std::string& instanceId, const std::string& key, const std::string& attribute, const char keySep = '.');

            /**
             * Return a property from a remote instance. The instance configuration
             * is internally cached, so it does not necessarily result in a query to the distributed system if
             * the device configuration has not changed since the last query.
             *
             * @param instanceId to retrieve the property from
             * @param key identifying the property
             * @param keySep path separator
             * @return the current property value on the remote device
             * @raise TypeException if the templated type does not match the property type.
             */
            template<class T>
            T get(const std::string& instanceId, const std::string& key, const char keySep = '.') {
                try {
                    const karabo::util::Hash::Attributes attrs = getDeviceSchema(instanceId).getParameterHash().getNode(key, keySep).getAttributes();
                    if (attrs.has(KARABO_SCHEMA_LEAF_TYPE)) {
                        const int leafType = attrs.get<int>(KARABO_SCHEMA_LEAF_TYPE);
                        if (leafType == karabo::util::Schema::STATE) {
                            if (typeid (T) == typeid (karabo::util::State)) {
                                return *reinterpret_cast<const T*> (&karabo::util::State::fromString(cacheAndGetConfiguration(instanceId).get<std::string>(key, keySep)));
                            }
                            throw KARABO_PARAMETER_EXCEPTION("State element at " + key + " may only return state objects");
                        }
                        if (leafType == karabo::util::Schema::ALARM_CONDITION) {
                            if (typeid (T) == typeid (karabo::util::AlarmCondition)) {
                                return *reinterpret_cast<const T*> (&karabo::util::AlarmCondition::fromString(cacheAndGetConfiguration(instanceId).get<std::string>(key, keySep)));
                            }
                            throw KARABO_PARAMETER_EXCEPTION("Alarm condition element at " + key + " may only return alarm condition objects");
                        }
                    }
                    return cacheAndGetConfiguration(instanceId).get<T > (key, keySep);
                } catch (const karabo::util::Exception& e) {

                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key + "\" from device \"" + instanceId + "\""));
                    return *static_cast<T*> (NULL); // never reached. Keep it to make the compiler happy.
                }
            }

            /**
             * Return a property from a remote instance. The instance configuration
             * is internally cached, so it does not necessarily result in a query to the distributed system if
             * the device configuration has not changed since the last query.
             *
             * @param instanceId to retrieve the property from
             * @param key identifying the property
             * @param value reference to write the property value to
             * @param keySep path separator
             * @raise TypeException if the templated type does not match the property type.
             */
            template<class T>
            void get(const std::string& instanceId, const std::string& key, T& value, const char keySep = '.') {
                try {
                    const karabo::util::Hash::Attributes attrs = getDeviceSchema(instanceId).getParameterHash().getNode(key, keySep).getAttributes();
                    if (attrs.has(KARABO_SCHEMA_LEAF_TYPE)) {
                        const int leafType = attrs.get<int>(KARABO_SCHEMA_LEAF_TYPE);
                        if (leafType == karabo::util::Schema::STATE) {
                            if (typeid (T) == typeid (karabo::util::State)) {
                                value = *reinterpret_cast<const T*> (&karabo::util::State::fromString(cacheAndGetConfiguration(instanceId).get<std::string>(key, keySep)));
                                return;
                            }
                            throw KARABO_PARAMETER_EXCEPTION("State element at " + key + " may only return state objects");
                        }
                        if (leafType == karabo::util::Schema::ALARM_CONDITION) {
                            if (typeid (T) == typeid (karabo::util::AlarmCondition)) {
                                value = *reinterpret_cast<const T*> (&karabo::util::AlarmCondition::fromString(cacheAndGetConfiguration(instanceId).get<std::string>(key, keySep)));
                                return;
                            }
                            throw KARABO_PARAMETER_EXCEPTION("Alarm condition element at " + key + " may only return alarm condition objects");
                        }
                    }
                    return cacheAndGetConfiguration(instanceId).get(key, value, keySep);
                } catch (const karabo::util::Exception& e) {

                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key + "\" from device \"" + instanceId + "\""));
                }
            }

            /**
             * Return a property from a remote instance casted to the template type. The instance configuration
             * is internally cached, so it does not necessarily result in a query to the distributed system if
             * the device configuration has not changed since the last query.
             *
             * @param instanceId to retrieve the property from
             * @param key identifying the property
             * @param keySep path separator
             * @return the current property value on the remote device
             * @raise TypeException if the property cannot be casted to the template type
             */
            template<class T>
            T getAs(const std::string& instanceId, const std::string& key, const char keySep = '.') {
                try {
                    return cacheAndGetConfiguration(instanceId).getAs<T > (key, keySep);
                } catch (const karabo::util::Exception& e) {

                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key + "\" from device \"" + instanceId + "\""));
                }
            }

            /**
             * Return a property from a remote instance as a boost::any value. The instance configuration
             * is internally cached, so it does not necessarily result in a query to the distributed system if
             * the device configuration has not changed since the last query.
             *
             * @param instanceId to retrieve the property from
             * @param key identifying the property
             * @param keySep path separator
             * @return the current property value on the remote device as boost::any type
             */
            boost::any getAsAny(const std::string& instanceId, const std::string& key, const char keySep = '.') {
                try {
                    return cacheAndGetConfiguration(instanceId).getNode(key, keySep).getValueAsAny();
                } catch (const karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key + "\" from device \"" + instanceId + "\""));
                }
                return boost::any();
            }


            /**
             * Toggles caching of the DataLogger map on (true) and off (false).
             * If set to true the logger map is always kept up to date, which
             * speeds up repeated calls to DeviceClient::getProperyHistory.
             * @param toggle
             * @return true if operation was successful
             */
            bool cacheLoggerMap(bool toggle);


            /**
             * Returns the history of a device property for a given period of time
             *
             * @param deviceId of the device holding the property
             * @param key path to the property on the device
             * @param from karabo::util::Epochstamp in Iso8601 format signifying the start
             *              of the time interval to get the history from
             * @param to karabo::util::Epochstamp in Iso8601 format signifying the end
             *              of the time interval to get the history from. If left empty default
             *              to now
             * @param maxNumData maximum number of data points to retrieve, starting from the
             *                   start of the interval
             * @return a vector of Hashes holding the property's history. Each entry consists
             *         of a Hash with a key "v" holding the value of the appropriate type. For
             *         each entry "v" Karabo train and timestamp attributes are set which can
             *         be retrieved using the karabo::util::Timestamp::fromHashAttributes method.
             */
             std::vector<karabo::util::Hash> getFromPast(const std::string& deviceId, const std::string& key, const std::string& from, std::string to = "", int maxNumData = 0);


            /**
             * Returns the history of a device property for a given period of time
             *
             * @param deviceId of the device holding the property
             * @param key path to the property on the device
             * @param from karabo::util::Epochstamp in Iso8601 format signifying the start
             *              of the time interval to get the history from
             * @param to karabo::util::Epochstamp in Iso8601 format signifying the end
             *              of the time interval to get the history from. If left empty default
             *              to now
             * @param maxNumData maximum number of data points to retrieve, starting from the
             *                   start of the interval
             * @return a vector of Hashes holding the property's history. Each entry consists
             *         of a Hash with a key "v" holding the value of the appropriate type. For
             *         each entry "v" Karabo train and timestamp attributes are set which can
             *         be retrieved using the karabo::util::Timestamp::fromHashAttributes method.
             */
            std::vector<karabo::util::Hash> getPropertyHistory(const std::string& deviceId, const std::string& key, const std::string& from, std::string to = "", int maxNumData = 0);


            /**
             * Returns instanceId of data log reader for data of given device. Could be empty.
             * @param deviceId
             * @return
             */
            std::string getDataLogReader(const std::string& deviceId);

            /**
             * Returns the device configuration and corresponding schema for a given
             * point in time. Information for the nearest matching logged time is returned.
             *
             * @param deviceId of the device to return the configuration for
             * @param timepoint to return information for. Should be an iso8601 formatted string.
             * @return a pair of the configuration Hash and corresponding device Schema
             */
            std::pair<karabo::util::Hash, karabo::util::Schema> getConfigurationFromPast(const std::string& deviceId, const std::string& timepoint);

            /**
             * Register a callback handler to be triggered if a new instance appears in the distributed system.
             * @param callBackFunction which will receive the instanceInfo Hash
             */
            void registerInstanceNewMonitor(const InstanceNewHandler& callBackFunction);

            /**
             * Register a callback handler to be triggered if an instance receives a state update from the distributed system
             * @param callBackFunction which will receive the instanceInfo Hash
             */
            void registerInstanceUpdatedMonitor(const InstanceUpdatedHandler& callBackFunction);

            /**
             * Register a callback handler to be triggered if an instance disappears from the distributed system
             * @param callBackFunction receiving the instanceId and instanceInfo Hash
             */
            void registerInstanceGoneMonitor(const InstanceGoneHandler& callBackFunction);

            /**
             * Register a callback handler to be triggered if an instance receives a schema update from the distributed system
             * @param callBackFunction receiving the instanceId and updated Schema
             */
            void registerSchemaUpdatedMonitor(const SchemaUpdatedHandler& callBackFunction);

            /**
             *  Register a callback handler to be triggered if a new class appears on a device server
             * @param callBackFunction receiving the server id, class id and new class Schema
             */
            void registerClassSchemaMonitor(const ClassSchemaHandler& callBackFunction);

            /**
             * Register a callback function to be triggered when a given property on a device in the distributed system updates
             * @param instanceId of the device to be monitored
             * @param key path to the property to be monitored
             * @param callbackFunction handling the update notification. It receives the device id, path, value and timestamp of the updated property
             * @return true if the operation was successful
             */
            template <class ValueType>
            bool registerPropertyMonitor(const std::string& instanceId, const std::string& key,
                                         const boost::function<void (const std::string& /*deviceId*/, const std::string& /*key*/, const ValueType& /*value*/, const karabo::util::Timestamp& /*timestamp*/) >& callbackFunction) {
                karabo::util::Schema schema = this->getDeviceSchema(instanceId);
                if (schema.has(key)) {
                    this->cacheAndGetConfiguration(instanceId);
                    {
                        boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._function", callbackFunction);
                    }
                    immortalize(instanceId);
                    return true;
                } else {
                    return false;
                }
            }

            /**
             * Register a callback function to be triggered when a given property on a device in the distributed system updates.
             * Additional user data may be passed to the callback
             * @param instanceId of the device to be monitored
             * @param key path to the property to be monitored
             * @param callbackFunction handling the update notification. It receives the device id, path, value and timestamp of the updated property
             *          as well as boost::any userData.
             * @param userData to be passed to the callback as boost::any
             * @return true if the operation was successful
             */
            template <class ValueType, class UserDataType>
            bool registerPropertyMonitor(const std::string& instanceId, const std::string& key, const boost::function<void ( const std::string& /*deviceId*/, const std::string& /*key*/,
                                                                                                                            const ValueType& /*value*/, const karabo::util::Timestamp& /*timestamp*/, const boost::any& /*userData*/) >& callbackFunction, const UserDataType& userData) {
                karabo::util::Schema schema = this->getDeviceSchema(instanceId);
                if (schema.has(key)) {
                    this->cacheAndGetConfiguration(instanceId);
                    {
                        boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._function", callbackFunction);
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._userData", userData);
                    }
                    immortalize(instanceId);
                    return true;
                } else {
                    return false;
                }
            }

            /**
             * Unregister a property monitor
             * @param instanceId to unregister the monitor from
             * @param key path to the property to unregister from.
             */
            void unregisterPropertyMonitor(const std::string& instanceId, const std::string& key);

            /**
             * Register a callback function to be triggered when a a device in the distributed system updates.
             * @param instanceId of the device to register to
             * @param callbackFunction handling the update. It will receive the device instance id and the updated device configuration Hash
             */
            void registerDeviceMonitor(const std::string& instanceId, const boost::function<void (const std::string&, const karabo::util::Hash&)>& callbackFunction);

            /**
             * Register a callback function to be triggered when a a device in the distributed system updates.
             * Additional user data may be passed to the callback
             *
             * @param instanceId of the device to register to
             * @param callbackFunction handling the update. It will receive the device instance id and the updated device configuration Hash as well as boost::any userData.
             * @param userData to be passed to the callback as boost::any
             */
            template <class UserDataType>
            void registerDeviceMonitor(const std::string& instanceId, const boost::function<void (const std::string&, const karabo::util::Hash&, const boost::any&)>& callbackFunction,
                                       const UserDataType& userData) {

                // It would be better to use stayConnected with async handlers as in the non-templated version of
                // registerDeviceMonitor - but since this version is probably not used at all (at least not in the
                // framework, there is no pressure to do so...
                stayConnected(instanceId);
                {
                    boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                    m_deviceChangedHandlers.set(instanceId + "._function", callbackFunction);
                    m_deviceChangedHandlers.set(instanceId + "._userData", userData);
                }
                m_signalSlotable.lock()->requestNoWait(instanceId, "slotGetSchema", "", "_slotSchemaUpdated", false);
                m_signalSlotable.lock()->requestNoWait(instanceId, "slotGetConfiguration", "", "_slotChanged");
                immortalize(instanceId);
            }

            /**
             * Unregister a device monitor
             * @param instanceId to unregister the monitor from
             */
            void unregisterDeviceMonitor(const std::string& instanceId);

            /**
             * Register handlers to be called whenever the defined output channel receives data or end-of-stream (EOS).
             * Internally, an InputChannel is created and configured using the cfg Hash.
             *
             * @param instanceId of the device having the output channel
             * @param channel is name of the output channel
             * @param dataHandler boost::function<void (const karabo::util::Hash&, const MetaData&) to be called whenever data arrives
             * @param inputChannelCfg configures via InputChanel::create(..) - use default except you know what your are doing
             *        for the expert:  "connectedOutputChannels" will be overwritten,
             *                         "onSlowness" default is overwritten to "drop"
             * @param eosHandler boost::function<void (const InputChannel::Pointer&)> called for EOS if given
             *
             * @return false if channel is already registered
             */
            bool registerChannelMonitor(const std::string& instanceId, const std::string& channel,
                                        const karabo::xms::SignalSlotable::DataHandler& dataHandler,
                                        const karabo::util::Hash& inputChannelCfg = karabo::util::Hash(),
                                        const karabo::xms::SignalSlotable::InputHandler& eosHandler = karabo::xms::SignalSlotable::InputHandler());

            /**
             * Register handlers to be called whenever the defined output channel receives data or end-of-stream (EOS).
             * Internally, an InputChannel is created and configured using the cfg Hash.
             *
             * @param channelName identifies the channel as a concatenation of the id of its devices, a colon (:) and
             *                     the name of the output channel (e.g. A/COOL/DEVICE:output)
             * @param dataHandler boost::function<void (const karabo::util::Hash&, const MetaData&) to be called whenever data arrives
             * @param inputChannelCfg configures via InputChanel::create(..) - use default except you know what your are doing
             *        for the expert:  "connectedOutputChannels" will be overwritten,
             *                         "onSlowness" default is overwritten to "drop"
             * @param eosHandler boost::function<void (const InputChannel::Pointer&)> called for EOS if given
             *
             * @return false if channel is already registered
             */
            bool registerChannelMonitor(const std::string& channelName,
                                        const karabo::xms::SignalSlotable::DataHandler& dataHandler,
                                        const karabo::util::Hash& inputChannelCfg = karabo::util::Hash(),
                                        const karabo::xms::SignalSlotable::InputHandler& eosHandler = karabo::xms::SignalSlotable::InputHandler());

            /**
             * Unregister monitoring of output channel
             *
             * @param instanceId of the device having the output channel
             * @param channel is name of the output channel
             * @return false if channel was not registered
             */
            bool unregisterChannelMonitor(const std::string& instanceId, const std::string& channel);

            /**
             * Unregister monitoring of output channel
             *
             * @param channelName identifies the channel as a concatenation of the id of its devices, a colon (:) and
             *                     the name of the output channel (e.g. A/COOL/DEVICE:output)
             * @return false if channel was not registered
             */
            bool unregisterChannelMonitor(const std::string& channelName);

            /**
             * Sets an attribute of a device.
             * @param deviceId The id of the device
             * @param key The parameter key (can be nested using "." as separator)
             * @param attributeKey The attribute key
             * @param attributeValue The attribute value
             * @param timeoutInSeconds Timeout in seconds, as this call is synchronous
             */
            template <class T>
            void setAttribute(const std::string& deviceId, const std::string& key,
                              const std::string& attributeKey, const T& attributeValue, int timeoutInSeconds = -1) {
                KARABO_GET_SHARED_FROM_WEAK(sp, m_signalSlotable);
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;
                karabo::util::Hash h("path", key, "attribute", attributeKey, "value", attributeValue);
                std::vector<karabo::util::Hash> v{h};
                sp->request(deviceId, "slotUpdateSchemaAttributes", v).timeout(timeoutInSeconds * 1000).receive();
            }

            /**
             * Set a remote property in the distributed system
             * @param instanceId of the device to set the property on
             * @param key path to the property to set
             * @param value to set
             * @param timeoutInSeconds maximum timeout until set operation fails, set to -1 to wait forever
             * @param keySep path separator
             */
            template <class T>
            void set(const std::string& instanceId, const std::string& key, const T& value, int timeoutInSeconds = -1, const char keySep = '.') {

                karabo::util::Hash tmp;
                tmp.set(key, value, keySep);
                set(instanceId, tmp, timeoutInSeconds);
            }

            /**
             * Set a remote property in the distributed system as a fire-and-forget operation.
             * Warning: there is no guarantee and indication if the set succeeded!
             * @param instanceId of the device to set the property on
             * @param key path to the property to set
             * @param value to set
             * @param keySep path separator
             */
            template <class T>
            void setNoWait(const std::string& instanceId, const std::string& key, const T& value, const char keySep = '.') {

                karabo::util::Hash tmp;
                tmp.set(key, value, keySep);
                setNoWait(instanceId, tmp);
            }

            /**
             * Bulk-set remote properties in the distributed system
             * @param instanceId of the device to set the property on
             * @param values a Hash containing the to be set value in a path structure indicating which properties to set
             * @param timeoutInSeconds maximum timeout until set operation fails, set to -1 to wait forever
             */
            void set(const std::string& instanceId, const karabo::util::Hash& values, int timeoutInSeconds = -1);

            /**
             * Bulk-set remote properties in the distributed system as a fire-and-forget operation.
             * Warning: there is no guarantee and indication if the set succeeded!
             * @param instanceId of the device to set the property on
             * @param values a Hash containing the to be set value in a path structure indicating which properties to set
             */
            void setNoWait(const std::string& instanceId, const karabo::util::Hash& values);

            /**
             * Executes a function on a device (an exposed via its Schema) and immediately returns (fire & forget)
             * @param deviceId The deviceId
             * @param command Name of the command
             */
            void executeNoWait(const std::string& deviceId, const std::string& command) {
                KARABO_GET_SHARED_FROM_WEAK(sp, m_signalSlotable);
                sp->call(deviceId, command);
            }

            /**
             * Executed a function on a device synchronously (waits until the function finished)
             * @param deviceId The devideId
             * @param command The command
             * @param timeoutInSeconds Timeout
             */
            void execute(const std::string& deviceId, const std::string& command, int timeoutInSeconds = -1) {
                KARABO_GET_SHARED_FROM_WEAK(sp, m_signalSlotable);
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;
                sp->request(deviceId, command).timeout(timeoutInSeconds * 1000).receive();
            }

            /**
             * Request the data schema for an output channel as a Hash containing relevant information
             * @param deviceId
             * @param outputChannelName
             * @return a Hash containing the output channel's data schema
             */
            karabo::util::Hash getOutputChannelSchema(const std::string & deviceId, const std::string& outputChannelName);

            /**
             * Get the list of all output channel names of the remote device.
             * @param deviceId
             * @return vector containing output channel names
             */
            std::vector<std::string> getOutputChannelNames(const std::string & deviceId);

            /**
             * Request locking of device at deviceId. Throws a karabo::util::LockException in case the lock cannot be acquired in the given timeout
             * @param deviceId: the device to be locked
             * @param recursive: if true, recursive locks on this device are allowed
             * @param timeout: timeout during which we try to acquire the lock. Set to -1 to wait try indefinitely, 0 to only try once,
             *                 otherwise give integer seconds to wait.
             * @return a Lock object, holding the lock to deviceId.
             */
            karabo::core::Lock lock(const std::string& deviceId, bool recursive = false, int timeout = -1);

            /**
             * Get all <i>properties</i> with the suitable <i>accessMode</i> exposed by <i>dataSourceId</i>.  
             * @param dataSourceId   data source containing properties
             * @param properties     properties that satisfy criteria below (output container)
             * @param accessMode     criteria used for filtering the data source's properties
             */
            void getDataSourceSchemaAsHash(const std::string& dataSourceId, karabo::util::Hash& properties,
                                           int accessMode = karabo::util::INIT|karabo::util::READ|karabo::util::WRITE);

        protected: // functions

            void initTopology();

            void cacheAvailableInstances();

            karabo::util::Hash prepareTopologyEntry(const std::string& instanceId, const karabo::util::Hash& instanceInfo) const;

            std::string prepareTopologyPath(const std::string& instanceId, const karabo::util::Hash& instanceInfo) const;

            void removeFromSystemTopology(const std::string& instanceId);

            virtual void setupSlots();

            virtual void _slotChanged(const karabo::util::Hash& hash, const std::string& instanceId);

            virtual void _slotInstanceNew(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            virtual void slotInstanceUpdated(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            virtual void _slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            virtual void _slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId);

            virtual void _slotClassSchema(const karabo::util::Schema& schema, const std::string& classId, const std::string& serverId);

            virtual void _slotLoggerMap(const karabo::util::Hash& loggerMap);

            static std::string generateOwnInstanceId();

            karabo::util::Schema cacheAndGetClassSchema(const std::string& serverId, const std::string& classId);

            karabo::util::Schema cacheAndGetDeviceSchema(const std::string& instanceId);

            karabo::util::Schema cacheAndGetActiveSchema(const std::string& instanceId);

            karabo::util::Hash cacheAndGetConfiguration(const std::string& instanceId);

            /*
             *  Keep connection to instanceId alive or establish if not there yet.
             *
             * If no handlers are given (default), do it synchronously, i.e. potentially block until connected.
             * Otherwise:
             *  - if connections are already established, just call asyncSuccessHandler (if not empty)
             *  - else request connection asynchronously using given handlers as success and failure call backs
             * Note that asyncFailureHandler works like an SignalSlotable::Requestor::AsyncErrorHandler, i.e. one
             * can make use of the "try { throw;} catch(..) {..}" pattern to get details of the problems.
             */
            void stayConnected(const std::string& instanceId,
                               const boost::function<void ()>& asyncSuccessHandler = boost::function<void ()>(),
                               const boost::function<void ()>& asyncFailureHandler = boost::function<void ()>());

            void eraseFromInstanceUsage(const std::string& instanceId);

            virtual void notifyDeviceChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId);

            virtual void notifyPropertyChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId);

            void castAndCall(const std::string& instanceId, const karabo::util::Hash& registered, const karabo::util::Hash& current, std::string path = "") const;

            void extractCommands(const karabo::util::Schema& schema, const std::string& parentKey, std::vector<std::string>& commands);

            std::vector<std::string> filterProperties(const karabo::util::Schema& schema, const int accessLevel);

            std::string getInstanceType(const karabo::util::Hash& instanceInfo) const;

            virtual void slotMasterPing();

            virtual void slotProvideSystemTopology();

            void age(const boost::system::error_code& e);

            void disconnectHandler(const std::string& signal, const std::string& instanceId,
                                   const std::vector<std::string>& toClear);

            void sendSignalsChanged();

            void immortalize(const std::string& deviceId);

            void mortalize(const std::string& deviceId);

            bool isImmortal(const std::string& deviceId) const;

            void mergeIntoRuntimeSystemDescription(const karabo::util::Hash& entry);

            bool existsInRuntimeSystemDescription(const std::string& path) const;

            /// returns true if path could be removed
            bool eraseFromRuntimeSystemDescription(const std::string& path);

            /// Get section (e.g. "device") from runtime description.
            /// Returns empty Hash if section does not exist.
            util::Hash getSectionFromRuntimeDescription(const std::string& section) const;

            /// Find full path of 'instanceId' in m_runtimeSystemDescription,
            /// empty if path does not exist.
            std::string findInstanceSafe(const std::string &instanceId) const;

        private:
            /// As findInstanceSafe, but to be called under protection of m_runtimeSystemDescriptionMutex.
            std::string findInstance(const std::string &instanceId) const;

            /// Actually process data in 'signalChangedMap' - try/catch should be outside.
            void doSendSignalsChanged(const SignalChangedMap &signalChangedMap);

            /// Marks 'instanceId' as used.
            /// Returns true if explicit "connect" call should still be done for it.
            bool connectNeeded(const std::string & instanceId);

            int getAccessLevel(const std::string& deviceId);

            void filterDataSchema(const std::string& deviceId, const karabo::util::Schema& schema, int accessMode, karabo::util::Hash& hash) const;

            void convertSchemaHash(const karabo::util::Hash& schemaHash, int accessMode, karabo::util::Hash & hash) const;

            void recursivelyAddCompoundDataTypes(const karabo::util::Hash& schemaHash, karabo::util::Hash & hash) const;
        };
    }
}


#endif

