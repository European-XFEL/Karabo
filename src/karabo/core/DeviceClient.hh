/*
 * $Id: Com.hh 6624 2012-06-27 12:57:09Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
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

#ifndef KARABO_CORE_DEVICE_CLIENT_HH
#define KARABO_CORE_DEVICE_CLIENT_HH

#include <atomic>
#include <boost/asio.hpp>
#include <karabo/core/InstanceChangeThrottler.hh>
#include <karabo/util/DataLogUtils.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

#include "karabo/data/time/Timestamp.hh"
#include "karabo/data/types/Schema.hh"


#define KARABO_GET_SHARED_FROM_WEAK(sp, wp) \
    auto sp = wp.lock();                    \
    if (!sp) throw KARABO_LOGIC_EXCEPTION(std::string(#wp) + " object already deleted");

namespace karabo {

    namespace core {

        // Forward
        class Device;

        /**
         * @class DeviceClient
         * @brief This class can be used to (remotely) control devices of the distributed system
         *        Synchronous calls (i.e. get()) are in fact asynchronous under the hood
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
        class DeviceClient : public std::enable_shared_from_this<DeviceClient> {
            friend class Device;

            /// keys are instance IDs, values are a sets of properties that changed
            typedef std::map<std::string, std::set<std::string>> SignalChangedMap;
            typedef std::function<void(const karabo::data::Hash& /*topologyEntry*/)> InstanceNewHandler;
            typedef std::function<void(const karabo::data::Hash& /*topologyEntry*/)> InstanceUpdatedHandler;
            typedef std::function<void(const std::string& /*instanceId*/, const karabo::data::Hash& /*instanceInfo*/)>
                  InstanceGoneHandler;
            typedef std::function<void(const std::string& /*deviceId*/, const karabo::data::Schema& /*schema*/)>
                  SchemaUpdatedHandler;
            typedef std::function<void(const std::string& /*serverId*/, const std::string& /*classId*/,
                                       const karabo::data::Schema& /*schema*/)>
                  ClassSchemaHandler;
            typedef std::function<void(const karabo::data::Hash& /* devicesChanges */)> DevicesChangedHandler;

            static const int CONNECTION_KEEP_ALIVE = 15; // keep in sync with GuiSever_Test.cc!

            std::shared_ptr<karabo::xms::SignalSlotable> m_internalSignalSlotable;

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
            karabo::data::Hash m_runtimeSystemDescription;

            mutable std::mutex m_runtimeSystemDescriptionMutex;

            std::weak_ptr<karabo::xms::SignalSlotable> m_signalSlotable;

            bool m_isShared;

            /// Map of devices that we are connected to with timer stating their age
            /// since last access. Before C++14 not an unordered_map since we want to erase while looping over it
            typedef std::map<std::string, int> InstanceUsage;
            InstanceUsage m_instanceUsage;

            karabo::data::Hash m_deviceChangedHandlers;

            /**
             * Handler for all monitored devices configuration updates during last interval.
             */
            DevicesChangedHandler m_devicesChangesHandler;
            std::mutex m_devicesChangesMutex;

            karabo::data::Hash m_propertyChangedHandlers;

            std::mutex m_instanceUsageMutex;

            std::mutex m_deviceChangedHandlersMutex;

            std::mutex m_propertyChangedHandlersMutex;

            int m_internalTimeout;

            std::atomic<bool> m_topologyInitialized;
            std::once_flag m_initTopologyOnce;

            boost::asio::steady_timer m_ageingTimer;

            static const unsigned int m_ageingIntervallMilliSec;

            bool m_getOlder; /// defines whether aging is running or not

            boost::asio::steady_timer m_signalsChangedTimer;
            bool m_runSignalsChangedTimer;
            std::atomic<long int> m_signalsChangedInterval;
            std::mutex m_signalsChangedMutex;
            SignalChangedMap m_signalsChanged; /// map of collected signalChanged

            std::mutex m_loggerMapMutex;

            data::Hash m_loggerMap;

            bool m_loggerMapCached;

            InstanceNewHandler m_instanceNewHandler;
            InstanceUpdatedHandler m_instanceUpdatedHandler;
            InstanceGoneHandler m_instanceGoneHandler;
            SchemaUpdatedHandler m_schemaUpdatedHandler;
            ClassSchemaHandler m_classSchemaHandler;

            std::shared_ptr<karabo::core::InstanceChangeThrottler> m_instanceChangeThrottler;

            std::set<std::string> m_immortals;
            mutable std::mutex m_immortalsMutex;

            std::string m_dataLoggerManagerId;
            std::string m_configManagerId;

           public:
            KARABO_CLASSINFO(DeviceClient, "DeviceClient", "1.2");

            /**
             * Constructor which establishes an own connection to the communication system.
             * This constructor is intended for stand-alone C++ device clients. Once we care about authentication,
             * this has to be added here.
             * @param instanceId The id with which the client should participate in the system.
             *                   If not unique or invalid, constructor will throw an exception.
             *                   If empty (i.e. default), an id will be generated from host name and process id.
             * @param implicitInit If true (default for backward compatibility - but NOT recommended!), the constructor
             *                     will implicitly try to trigger a call to initialize() via the event loop. Since this
             *                     can fail silently, it is strongly recommended to use implicitInit = false and
             *                     call the initialize() method right after the constructor.
             * @param serviceDeviceIds A hash with ids of core service devices; e.g, "dataLoggerManagerId" key and the
             *                         value is the name of the DataLoggerManager the device client instance should use
             *                         for data logging operations. Currently keys "dataLoggerManagerId" and
             *                         "configurationManagerId" are supported.
             */
            explicit DeviceClient(const std::string& instanceId = std::string(), bool implicitInit = true,
                                  const karabo::data::Hash& serviceDeviceIds = karabo::data::Hash());

            /**
             * Constructor using instantiated signalSlotable object (shared communication - take care that the
             * signalSlotable is kept alive since the DeviceClient will only keep a weak pointer)
             * @param signalSlotable An instance of the SignalSlotable lass
             * @param implicitInit If true (default for backward compatibility - but NOT recommended!), the constructor
             *                     will implicitly try to trigger a call to initialize() via the event loop. Since this
             *                     can fail silently, it is strongly recommended to use implicitInit = false and
             *                     call the initialize() method right after the constructor.
             * @param serviceDeviceIds A hash with ids of core service devices; e.g, "dataLoggerManagerId" key and the
             *                         value is the name of the DataLoggerManager the device client instance should use
             *                         for data logging operations. Currently keys "dataLoggerManagerId" and
             *                         "configurationManagerId" are supported.
             */
            explicit DeviceClient(const std::shared_ptr<karabo::xms::SignalSlotable>& signalSlotable,
                                  bool implicitInit = true,
                                  const karabo::data::Hash& serviceDeviceIds = karabo::data::Hash());

            /**
             * Constructor aimed at cases where a specific DataLoggerManagerId is required.
             * Requires an explicit call to DeviceClient::initialize() after the construction takes place.
             *
             * @param instanceId The id with which the client should participate in the system.
             *                   If not unique or invalid, constructor will throw an exception.
             *                   If empty, an id will be generated from host name and process id.
             * @param serviceDeviceIds A hash with ids of core service devices; e.g, "dataLoggerManagerId" key and the
             *                         value is the name of the DataLoggerManager the device client instance should use
             *                         for data logging operations. Currently keys "dataLoggerManagerId" and
             *                         "configurationManagerId" are supported.
             */
            DeviceClient(const std::string& instanceId, const karabo::data::Hash& serviceDeviceIds);

            /**
             * Constructor using instantiated signalSlotable object (shared communication - take care that the
             * signalSlotable is kept alive since the DeviceClient will only keep a weak pointer) and aimed at cases
             * where a specific DataLoggerManagerId is required. Requires an explicit call to DeviceClient::initialize()
             * after the construction takes place.
             *
             * @param signalSlotable An instance of the SignalSlotable lass
             * @param serviceDeviceIds A hash with ids of core service devices; e.g, "dataLoggerManagerId" key and the
             *                         value is the name of the DataLoggerManager the device client instance should use
             *                         for data logging operations. Currently keys "dataLoggerManagerId" and
             *                         "configurationManagerId" are supported.
             */
            DeviceClient(const std::shared_ptr<karabo::xms::SignalSlotable>& signalSlotable,
                         const karabo::data::Hash& serviceDeviceIds);

            virtual ~DeviceClient();

            /**
             * Second constructor.
             * It is strongly recommended to use the constructors with implicitInit = false
             * and explicitely call initialize() after the construction.
             */
            void initialize();

            /**
             * InstanceId of underlying communication object (i.e. SignalSlotable)
             */
            const std::string& getInstanceId() {
                karabo::xms::SignalSlotable::Pointer ptr(m_signalSlotable); // throws if nothing behind weak_ptr
                return ptr->getInstanceId();
            }

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
            karabo::data::Hash getSystemInformation();


            /**
             * Returns only the topology of the current system (no instance configurations or descriptions)
             * @return Hash containing the topology of the runtime system
             */
            karabo::data::Hash getSystemTopology();

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
            karabo::data::Schema getDeviceSchema(const std::string& instanceId);


            /**
             * Retrieves the full Schema (parameter description) of the given instance
             * The call is non-blocking, if no Schema is currently available the return
             * will be empty. However, the schema request will be sent and should lead to
             * later arrival of a schema.
             * @param instanceId Device's instance ID
             * @return full Schema
             */
            karabo::data::Schema getDeviceSchemaNoWait(const std::string& instanceId);

            /**
             * Retrieves the currently active Schema (filtered by allowed states and allowed roles)
             * of the given instance
             * @param instanceId Device's instance ID
             * @return active Schema
             */
            karabo::data::Schema getActiveSchema(const std::string& instanceId);

            /**
             * Retrieves a schema from static context of a loaded Device class plug-in.
             * This schema represents a description of parameters possible to configure for instantiation.
             * I.e. returns in fact a description of the constructor arguments to that device class.
             * @param serverId instanceId of a deviceServer
             * @param classId name of loaded class on the deviceServer (classId)
             * @return Schema describing parameters available at instantiation time
             */
            karabo::data::Schema getClassSchema(const std::string& serverId, const std::string& classId);

            /**
             * Retrieves a schema from static context of a loaded Device class plug-in.
             * This schema represents a description of parameters possible to configure for instantiation.
             * This function can be used to pre-cache a schema for later usage. It returns an empty schema.
             * @param serverId instanceId of a deviceServer
             * @param classId name of loaded class on the deviceServer (classId)
             * @return an empty schem
             */
            karabo::data::Schema getClassSchemaNoWait(const std::string& serverId, const std::string& classId);

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
            karabo::data::Hash loadConfigurationFromFile(const std::string& filename);

            /**
             * Attempt to instantiate a device of the specified class, on a remote server with a given initial
             * configuration
             *
             * @param serverInstanceId of the server to instantiate the device on. Needs to have the device plugin
             * available
             * @param classId of the device to be instantiate
             * @param configuration Hash which contains the initial device configuration. It must have one out of the
             * two following forms: option 1:
             *               - key "classId" pointing to a string,
             *          option 2:
             *               - no classId specified: class id to be instantiated is taken from classId parameter
             *          option 3 (for backward compatibility - not recommended):
             *               - a single key (e.g. "myClassId") representing the classId
             *               - the value for this key is a Hash with all the non-default properties
             *
             * @param timeoutInSeconds by default set to -1, which means block indefinitely, if a positive value an
             * Exception is thrown if the device hasn't been instantiated.
             * @return (ok, reply) pair where ok is true if no exception occurred and reply is the answer received from
             * server
             */
            std::pair<bool, std::string> instantiate(const std::string& serverInstanceId, const std::string& classId,
                                                     const karabo::data::Hash& configuration = karabo::data::Hash(),
                                                     int timeoutInSeconds = -1);

            /**
             * Instantiate a device on a remote server
             * @param serverInstanceId of the server to instantiate the device on. Needs to have the device plugin
             * available
             * @param configuration Hash which contains the initial device configuration. The 'classId' attribute must
             * be present.
             * @param timeoutInSeconds by default set to -1, which means block indefinitely, if a positive value an
             * Exception is thrown if the device hasn't been instantiated.
             * @return
             */
            std::pair<bool, std::string> instantiate(const std::string& serverInstanceId,
                                                     const karabo::data::Hash& configuration,
                                                     int timeoutInSeconds = -1);


            /**
             * Utility method that takes care of adding classId to configuration of device to be instantiated
             * by instantiate and instantiateNoWait. If configuration does not have 'classId' key, this is added,
             * with the value of classId parameter. Otherwise the configuration 'classId' value is used.
             * In the latter case, if the value of classId parameter mismatches the one of 'classId' attribute of
             * configuration a warning is thrown.
             * @param classId of the device to be instantiated.
             * @param configuration of the device to be instantiated.
             * @return configuration ready to be sent to device server
             */
            karabo::data::Hash formatConfigToInstantiate(const std::string& classId,
                                                         const karabo::data::Hash& configuration);

            /**
             * Instantiate a device on a remote server. In contrast to DeviceClient::instantiate, this function returns
             * immediately.
             *
             * @param serverInstanceId of the server to instantiate the device on. Needs to have the device plugin
             * available
             * @param classId of the device to be instantiate
             * @param configuration Hash which contains the initial device configuration. It must have one out of the
             * two following forms: option 1:
             *               - key "classId" pointing to a string,
             *          option 2:
             *               - no classId specified: class id to be instantiated is taken from classId parameter
             *          option 3 (for backward compatibility - not recommended):
             *               - a single key (e.g. "myClassId") representing the classId
             *               - the value for this key is a Hash with all the non-default properties
             *
             */
            void instantiateNoWait(const std::string& serverInstanceId, const std::string& classId,
                                   const karabo::data::Hash& configuration = karabo::data::Hash());

            /**
             * Instantiate a device on a remote server. In contrast to DeviceClient::instantiate, this function returns
             * immediately.
             * @param serverInstanceId of the server to instantiate the device on. Needs to have the device plugin
             * available
             * @param configuration Hash which contains the initial device configuration. The 'classId' attribute must
             * be present.
             * @return
             */
            void instantiateNoWait(const std::string& serverInstanceId, const karabo::data::Hash& configuration);

            /**
             * Kill a device in the distributed system and wait until it is actually dead
             * @param deviceId of the device to kill
             * @param timeoutInSeconds timeoutInSeconds by default set to -1, which means block indefinitely, if a
             * positive value an Exception is thrown if the device hasn't been killed.
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
             * @param timeoutInSeconds timeoutInSeconds timeoutInSeconds by default set to -1, which means block
             * indefinitely, if a positive value an Exception is thrown if the device server hasn't been killed.
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
            karabo::data::Hash get(const std::string& instanceId);

            /**
             * Return the configuration Hash of an instance. The configuration
             * is internally cached, so it does not necessarily result in a query to the distributed system if
             * the device configuration has not changed since the last query.
             *
             * @param instanceId for which to return the configuration of
             * @param hash reference to write configuration into
             */
            void get(const std::string& instanceId, karabo::data::Hash& hash);

            /**
             * Return the cached configuration if it is still valid, otherwise query an updated version
             * but return an empty Hash.
             * @param deviceId for which to return the configuration of
             * @return a Hash holding the instance configuration
             */
            karabo::data::Hash getConfigurationNoWait(const std::string& deviceId);

            /**
             * Check if an attribute exists for a property on a given instance
             * @param instanceId to check on
             * @param key path to the property to check if it has a given attribute
             * @param attribute to check for
             * @param keySep path separator
             * @return a boolean indicating if the attribute is present
             */
            bool hasAttribute(const std::string& instanceId, const std::string& key, const std::string& attribute,
                              const char keySep = data::Hash::k_defaultSep);

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
            template <class T>
            T get(const std::string& instanceId, const std::string& key, const char keySep = data::Hash::k_defaultSep) {
                try {
                    const karabo::data::Hash::Attributes attrs =
                          getDeviceSchema(instanceId).getParameterHash().getNode(key, keySep).getAttributes();
                    if (attrs.has(KARABO_SCHEMA_CLASS_ID)) {
                        const std::string& classId = attrs.get<std::string>(KARABO_SCHEMA_CLASS_ID);
                        if (classId == "State") {
                            if (typeid(T) == typeid(karabo::data::State)) {
                                return *reinterpret_cast<const T*>(&karabo::data::State::fromString(
                                      cacheAndGetConfiguration(instanceId).get<std::string>(key, keySep)));
                            }
                            throw KARABO_PARAMETER_EXCEPTION("State element at " + key +
                                                             " may only return state objects");
                        }
                        if (classId == "AlarmCondition") {
                            if (typeid(T) == typeid(karabo::data::AlarmCondition)) {
                                return *reinterpret_cast<const T*>(&karabo::data::AlarmCondition::fromString(
                                      cacheAndGetConfiguration(instanceId).get<std::string>(key, keySep)));
                            }
                            throw KARABO_PARAMETER_EXCEPTION("Alarm condition element at " + key +
                                                             " may only return alarm condition objects");
                        }
                    }
                    return cacheAndGetConfiguration(instanceId).get<T>(key, keySep);
                } catch (const karabo::data::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key +
                                                                 "\" from device \"" + instanceId + "\""));
                    return *static_cast<T*>(NULL); // never reached. Keep it to make the compiler happy.
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
            template <class T>
            void get(const std::string& instanceId, const std::string& key, T& value,
                     const char keySep = data::Hash::k_defaultSep) {
                try {
                    const karabo::data::Hash::Attributes attrs =
                          getDeviceSchema(instanceId).getParameterHash().getNode(key, keySep).getAttributes();
                    if (attrs.has(KARABO_SCHEMA_CLASS_ID)) {
                        const std::string& classId = attrs.get<std::string>(KARABO_SCHEMA_CLASS_ID);
                        if (classId == "State") {
                            if (typeid(T) == typeid(karabo::data::State)) {
                                value = *reinterpret_cast<const T*>(&karabo::data::State::fromString(
                                      cacheAndGetConfiguration(instanceId).get<std::string>(key, keySep)));
                                return;
                            }
                            throw KARABO_PARAMETER_EXCEPTION("State element at " + key +
                                                             " may only return state objects");
                        }
                        if (classId == "AlarmCondition") {
                            if (typeid(T) == typeid(karabo::data::AlarmCondition)) {
                                value = *reinterpret_cast<const T*>(&karabo::data::AlarmCondition::fromString(
                                      cacheAndGetConfiguration(instanceId).get<std::string>(key, keySep)));
                                return;
                            }
                            throw KARABO_PARAMETER_EXCEPTION("Alarm condition element at " + key +
                                                             " may only return alarm condition objects");
                        }
                    }
                    return cacheAndGetConfiguration(instanceId).get(key, value, keySep);
                } catch (const karabo::data::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key +
                                                                 "\" from device \"" + instanceId + "\""));
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
            template <class T>
            T getAs(const std::string& instanceId, const std::string& key,
                    const char keySep = data::Hash::k_defaultSep) {
                try {
                    return cacheAndGetConfiguration(instanceId).getAs<T>(key, keySep);
                } catch (const karabo::data::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key +
                                                                 "\" from device \"" + instanceId + "\""));
                    // Please compiler by adding a (crashing...) return statement that is never reached:
                    return *static_cast<T*>(nullptr);
                }
            }

            /**
             * Return a property from a remote instance as a std::any value. The instance configuration
             * is internally cached, so it does not necessarily result in a query to the distributed system if
             * the device configuration has not changed since the last query.
             *
             * @param instanceId to retrieve the property from
             * @param key identifying the property
             * @param keySep path separator
             * @return the current property value on the remote device as std::any type
             */
            std::any getAsAny(const std::string& instanceId, const std::string& key,
                              const char keySep = data::Hash::k_defaultSep) {
                try {
                    return cacheAndGetConfiguration(instanceId).getNode(key, keySep).getValueAsAny();
                } catch (const karabo::data::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key +
                                                                 "\" from device \"" + instanceId + "\""));
                }
                return std::any();
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
             * @param from karabo::data::Epochstamp in Iso8601 format signifying the start
             *              of the time interval to get the history from
             * @param to karabo::data::Epochstamp in Iso8601 format signifying the end
             *              of the time interval to get the history from. If left empty default
             *              to now
             * @param maxNumData maximum number of data points to retrieve, starting from the
             *                   start of the interval
             * @return a vector of Hashes holding the property's history. Each entry consists
             *         of a Hash with a key "v" holding the value of the appropriate type. For
             *         each entry "v" Karabo train and timestamp attributes are set which can
             *         be retrieved using the karabo::data::Timestamp::fromHashAttributes method.
             */
            std::vector<karabo::data::Hash> getFromPast(const std::string& deviceId, const std::string& key,
                                                        const std::string& from, std::string to = "",
                                                        int maxNumData = 0);


            /**
             * Returns the history of a device property for a given period of time
             *
             * @param deviceId of the device holding the property
             * @param key path to the property on the device
             * @param from karabo::data::Epochstamp in Iso8601 format signifying the start
             *              of the time interval to get the history from
             * @param to karabo::data::Epochstamp in Iso8601 format signifying the end
             *              of the time interval to get the history from. If left empty default
             *              to now
             * @param maxNumData maximum number of data points to retrieve, starting from the
             *                   start of the interval
             * @return a vector of Hashes holding the property's history. Each entry consists
             *         of a Hash with a key "v" holding the value of the appropriate type. For
             *         each entry "v" Karabo train and timestamp attributes are set which can
             *         be retrieved using the karabo::data::Timestamp::fromHashAttributes method.
             */
            std::vector<karabo::data::Hash> getPropertyHistory(const std::string& deviceId, const std::string& key,
                                                               const std::string& from, std::string to = "",
                                                               int maxNumData = 0);


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
            std::pair<karabo::data::Hash, karabo::data::Schema> getConfigurationFromPast(const std::string& deviceId,
                                                                                         const std::string& timepoint);

            /**
             * Returns the configurations saved for a device under names that contain a given name part.
             *
             * @param deviceId of the device whose named configuration(s) and schema(s) should be returned.
             * @param namePart of the device configuration(s) and schema(s) to be returned. An empty namePart means
             *                 returns all the named configuration(s) and schema(s)
             * @return a hash with the operation execution status and the list of configuration(s) and schema(s) in
             *         case of success. For the operation execution status, the returned hash has the keys "success"
             *         with a boolean value that indicates whether the the operation was successful and a key
             *         "reason" with a string value that will contain the reason for failure or will be empty
             *         in the case of success. The returned hash will also have a key "configs" whose value will be
             *         a vector of hashes with data about the configs that match the name part. If no configuration
             *         is saved for the device under a name that contains the namePart, the "configs" vector will be
             *         empty. Each hash in the "configs" vector contains the keys "name", "timepoint".
             */
            karabo::data::Hash listInitConfigurations(const std::string& deviceId, const std::string& namePart = "");

            /**
             * Returns the configuration and schema saved for a device under a given name.
             *
             * @param deviceId of the device whose named configuration and schema should be returned.
             * @param name of the device configuration and schema to be returned.
             * @return a hash with the operation execution status and the device configuration and schema in
             *         case of success. For the operation execution status, the returned hash has the keys
             *         "success" with a boolean value that indicates whether the the operation was successful and
             *         a key "reason" with a string value that will contain the reason for failure or will
             *         be empty in the case of success. The returned hash will also have a key "config" whose value
             *         will be a hash with the keys "name", "timepoint", "description", "priority", "user", "config"
             *         and "schema" when a device configuration with the given name is found or an empty hash in case
             *         of failure or when no device configuration with the given name exists.
             */
            karabo::data::Hash getInitConfiguration(const std::string& deviceId, const std::string& name);


            /**
             * Saves a collection of current device configurations (and the corresponding schemas) in the
             * configuration database under a common name, user, priority and description.
             *
             * @param name to be assigned to the saved collection of device configurations (with schemas).
             * @param deviceIds the devices whose current configurations (and schemas) are to be saved.
             * @return a pair with a success flag (true when the operation succeeds) in the first position and
             *         a reason failture description (empty in case of success) in the second position.
             *
             */
            std::pair<bool, std::string> saveInitConfiguration(const std::string& name,
                                                               const std::vector<std::string>& deviceIds);

            /**
             * Register a throttled callback handler to be triggered when a new device instance appears, updates its
             * instance info record or goes away in the distributed system. The throtter that dispatches the instance
             * changes events to the handler uses a given interval between its running cycles.
             *
             * @param callBackFunction Function to be invoked with information about the instances changes events.
             * @param throttlerInterval Interval, in milliseconds, between successive cycles of the throttle.
             * @param maxChangesPerCycle Maximum number of instance changes to be dispatched per cycle of the throttler
             * - upon reaching this limit the throttler immediately dispatches the changes, despite the elapsed time
             * from the last cycle.
             */
            void registerInstanceChangeMonitor(const InstanceChangeThrottler::InstanceChangeHandler& callBackFunction,
                                               unsigned int throttlerIntervalMs = 500u,
                                               unsigned int maxChangesPerCycle = 100u);

            /**
             * Flushes, asap, the throttled instance changes that are waiting to be dispatched.
             *
             */
            void flushThrottledInstanceChanges();

            /**
             * Register a callback handler to be triggered if a new instance appears in the distributed system.
             * @param callBackFunction which will receive the instanceInfo Hash
             */
            void registerInstanceNewMonitor(const InstanceNewHandler& callBackFunction);

            /**
             * Register a callback handler to be triggered if an instance receives a state update from the distributed
             * system
             * @param callBackFunction which will receive the instanceInfo Hash
             */
            void registerInstanceUpdatedMonitor(const InstanceUpdatedHandler& callBackFunction);

            /**
             * Register a callback handler to be triggered if an instance disappears from the distributed system
             * @param callBackFunction receiving the instanceId and instanceInfo Hash
             */
            void registerInstanceGoneMonitor(const InstanceGoneHandler& callBackFunction);

            /**
             * Register a callback handler to be triggered if an instance receives a schema update from the distributed
             * system
             * @param callBackFunction receiving the instanceId and updated Schema
             *
             * @note Currently, registering only a schema update monitor with an instance
             *       of a DeviceClient is not enough to have the registered call-back activated.
             *       A workaround for this is to also register a property monitor with the
             *       same instance of DeviceClient that has been used to register the schema
             *       update monitor.
             *
             *       Example:
             *
             *       DeviceClient dc = std::shared_ptr<DeviceClient>(new DeviceClient());
             *       dc->registerSchemaUpdateMonitor(fnSchemaUpdateHandler);
             *       dc->registerPropertyMonitor("deviceId", "property_to_monitor", fnCallback);
             */
            void registerSchemaUpdatedMonitor(const SchemaUpdatedHandler& callBackFunction);

            /**
             *  Register a callback handler to be triggered if a new class appears on a device server
             * @param callBackFunction receiving the server id, class id and new class Schema
             */
            void registerClassSchemaMonitor(const ClassSchemaHandler& callBackFunction);

            /**
             * Register a callback function to be triggered when a given property on a device in the distributed system
             * updates
             * @param instanceId of the device to be monitored
             * @param key path to the property to be monitored
             * @param callbackFunction handling the update notification. It receives the device id, path, value and
             * timestamp of the updated property
             * @return true if the operation was successful
             */
            template <class ValueType>
            bool registerPropertyMonitor(
                  const std::string& instanceId, const std::string& key,
                  const std::function<void(const std::string& /*deviceId*/, const std::string& /*key*/,
                                           const ValueType& /*value*/, const karabo::data::Timestamp& /*timestamp*/)>&
                        callbackFunction) {
                karabo::data::Schema schema = this->getDeviceSchema(instanceId);
                if (schema.has(key)) {
                    this->cacheAndGetConfiguration(instanceId);
                    {
                        std::lock_guard<std::mutex> lock(m_propertyChangedHandlersMutex);
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._function", callbackFunction);
                    }
                    immortalize(instanceId);
                    return true;
                } else {
                    return false;
                }
            }

            /**
             * Register a callback function to be triggered when a given property on a device in the distributed system
             * updates. Additional user data may be passed to the callback
             * @param instanceId of the device to be monitored
             * @param key path to the property to be monitored
             * @param callbackFunction handling the update notification. It receives the device id, path, value and
             * timestamp of the updated property as well as std::any userData.
             * @param userData to be passed to the callback as std::any
             * @return true if the operation was successful
             */
            template <class ValueType, class UserDataType>
            bool registerPropertyMonitor(
                  const std::string& instanceId, const std::string& key,
                  const std::function<void(const std::string& /*deviceId*/, const std::string& /*key*/,
                                           const ValueType& /*value*/, const karabo::data::Timestamp& /*timestamp*/,
                                           const std::any& /*userData*/)>& callbackFunction,
                  const UserDataType& userData) {
                karabo::data::Schema schema = this->getDeviceSchema(instanceId);
                if (schema.has(key)) {
                    this->cacheAndGetConfiguration(instanceId);
                    {
                        std::lock_guard<std::mutex> lock(m_propertyChangedHandlersMutex);
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
             *
             * @param instanceId of the device to register to
             * @param callbackFunction handling the update. It will receive the device instance id and the updated
             * device configuration Hash
             */
            void registerDeviceMonitor(
                  const std::string& instanceId,
                  const std::function<void(const std::string&, const karabo::data::Hash&)>& callbackFunction);

            /**
             * Registers a device to have its configurations changes monitored.
             *
             * @param deviceId of the device to be added to the set of monitored devices.
             *
             * @note In order to receive notifications about configuration changes for any of the
             * monitored devices, one needs to register handlers by calling registerDeviceMonitor
             * (updates one by one - even if updates are throttled) or with registerDevicesMonitor
             * (bulk updates).
             */
            void registerDeviceForMonitoring(const std::string& deviceId);

            /**
             * Registers a handler for configuration changes for any of the monitored devices.
             *
             * @note * To register a device to be monitored, a call to registerDeviceForMonitoring must be made.
             *       * Throttling of device updates must be enabled via a call to setDeviceMonitorInterval with an
             *         argument greater than 0.
             *
             * @param devicesChangesHandler callback function for configuration changes events for any monitored device.
             */
            void registerDevicesMonitor(const DevicesChangedHandler& devicesChangedHandler);

            /**
             * Unregisters a device from configuration changes monitoring.
             *
             * @param deviceId of the device to be removed from the set of monitored devices.
             */
            void unregisterDeviceFromMonitoring(const std::string& deviceId);

            /**
             * Register a callback function to be triggered when a a device in the distributed system updates.
             * Additional user data may be passed to the callback
             *
             * @param instanceId of the device to register to
             * @param callbackFunction handling the update. It will receive the device instance id and the updated
             * device configuration Hash as well as std::any userData.
             * @param userData to be passed to the callback as std::any
             */
            template <class UserDataType>
            void registerDeviceMonitor(const std::string& instanceId,
                                       const std::function<void(const std::string&, const karabo::data::Hash&,
                                                                const std::any&)>& callbackFunction,
                                       const UserDataType& userData) {
                // It would be better to use stayConnected with async handlers as in the non-templated version of
                // registerDeviceMonitor - but since this version is probably not used at all (at least not in the
                // framework, there is no pressure to do so...
                stayConnected(instanceId);
                {
                    std::lock_guard<std::mutex> lock(m_deviceChangedHandlersMutex);
                    m_deviceChangedHandlers.set(instanceId + "._function", callbackFunction);
                    m_deviceChangedHandlers.set(instanceId + "._userData", userData);
                }
                m_signalSlotable.lock()->requestNoWait(instanceId, "slotGetSchema", "_slotSchemaUpdated", false);
                m_signalSlotable.lock()->requestNoWait(instanceId, "slotGetConfiguration", "_slotChanged");
                immortalize(instanceId);
            }

            /**
             * Unregister a device monitor.
             * @param instanceId to unregister the monitor from
             */
            void unregisterDeviceMonitor(const std::string& instanceId);

            /**
             * Container of handlers for InputChannel, to be passed to
             *
             * bool registerChannelMonitor(const std::string& channelName, const InputChannelHandlers& handlers,
             *                             const karabo::data::Hash& inputChannelCfg = karabo::data::Hash());
             *
             * See documentation of that method for meaning of various handlers.
             */
            struct InputChannelHandlers {
                InputChannelHandlers(){
                      // members are correctly initialised by their default empty constructors
                };

                /**
                 * Construct with all handlers except input handler (could be specified afterwards)
                 */
                explicit InputChannelHandlers(const karabo::xms::SignalSlotable::DataHandler& data,
                                              const karabo::xms::SignalSlotable::InputHandler& eos =
                                                    karabo::xms::SignalSlotable::InputHandler(),
                                              const std::function<void(karabo::net::ConnectionStatus)>& status =
                                                    std::function<void(karabo::net::ConnectionStatus)>())
                    : dataHandler(data), inputHandler(), eosHandler(eos), statusTracker(status){};

                /**
                 * Construct with all handlers except data handler (could be specified afterwards)
                 */
                explicit InputChannelHandlers(const karabo::xms::SignalSlotable::InputHandler& input,
                                              const karabo::xms::SignalSlotable::InputHandler& eos =
                                                    karabo::xms::SignalSlotable::InputHandler(),
                                              const std::function<void(karabo::net::ConnectionStatus)>& status =
                                                    std::function<void(karabo::net::ConnectionStatus)>())
                    : dataHandler(), inputHandler(input), eosHandler(eos), statusTracker(status){};

                // Could switch to 'karabo::xms::InputChannel::Handlers streamHandlers;', replacing the following three
                // - but that breaks the API...
                karabo::xms::SignalSlotable::DataHandler dataHandler;
                karabo::xms::SignalSlotable::InputHandler inputHandler;
                karabo::xms::SignalSlotable::InputHandler eosHandler;
                std::function<void(karabo::net::ConnectionStatus)> statusTracker;
            };

            /**
             * Register handlers to be called whenever the defined output channel receives data or end-of-stream (EOS).
             * Internally, an InputChannel is created and configured using the cfg Hash and its connection status
             * can be monitored via the 'statusTracker' of the handlers argument
             *
             * @param channelName identifies the channel as a concatenation of the id of its devices, a colon (:) and
             *                    the name of the output channel (e.g. A/COOL/DEVICE:output)
             * @param handlers container for various handlers (handlers can be empty function pointers):
             *                 - dataHandler std::function<void (const karabo::data::Hash&, const MetaData&)> to be
             *                               called whenever data arrives
             *                 - inputHandler std::function<void (const InputChannel::Pointer&)> to be called whenever
             *                                data arrives
             *                 - eosHandler std::function<void (const InputChannel::Pointer&)> called for EOS
             *                 - statusTracker std::function<void(karabo::net::ConnectionStatus)> called whenever
             *                                 the connection status of the underlying InputChannel changes
             * @param inputChannelCfg configures via InputChanel::create(..) - use default except you know what your are
             *                        doing.
             *                        For the expert: "connectedOutputChannels" will be overwritten
             *
             * @return false if channel is already registered
             */
            bool registerChannelMonitor(const std::string& channelName, const InputChannelHandlers& handlers,
                                        const karabo::data::Hash& inputChannelCfg = karabo::data::Hash());

            /**
             * Register handlers to be called whenever the defined output channel receives data or end-of-stream (EOS).
             *
             * DEPRECATED - use interface with 'InputChannelHandlers' argument!
             *
             * @param instanceId of the device having the output channel
             * @param channel is name of the output channel
             * @param dataHandler std::function<void (const karabo::data::Hash&, const MetaData&) to be called
             * whenever data arrives
             * @param inputChannelCfg configures via InputChanel::create(..) - use default except you know what your are
             * doing for the expert:  "connectedOutputChannels" will be overwritten
             * @param eosHandler std::function<void (const InputChannel::Pointer&)> called for EOS if given
             * @param inputHandler std::function<void (const InputChannel::Pointer&)> to be called whenever data
             * arrives
             *
             * @return false if channel is already registered
             */
            bool registerChannelMonitor(const std::string& instanceId, const std::string& channel,
                                        const karabo::xms::SignalSlotable::DataHandler& dataHandler,
                                        const karabo::data::Hash& inputChannelCfg = karabo::data::Hash(),
                                        const karabo::xms::SignalSlotable::InputHandler& eosHandler =
                                              karabo::xms::SignalSlotable::InputHandler(),
                                        const karabo::xms::SignalSlotable::InputHandler& inputHandler =
                                              karabo::xms::SignalSlotable::InputHandler());

            /**
             * Register handlers to be called whenever the defined output channel receives data or end-of-stream (EOS).
             *
             * DEPRECATED - use interface with 'InputChannelHandlers' argument!
             *
             * @param channelName identifies the channel as a concatenation of the id of its devices, a colon (:) and
             *                     the name of the output channel (e.g. A/COOL/DEVICE:output)
             * @param dataHandler std::function<void (const karabo::data::Hash&, const MetaData&) to be called
             * whenever data arrives
             * @param inputChannelCfg configures via InputChanel::create(..) - use default except you know what your are
             * doing for the expert:  "connectedOutputChannels" will be overwritten
             * @param eosHandler std::function<void (const InputChannel::Pointer&)> called for EOS if given
             * @param inputHandler std::function<void (const InputChannel::Pointer&)> to be called whenever data
             * arrives
             *
             * @return false if channel is already registered
             */
            bool registerChannelMonitor(const std::string& channelName,
                                        const karabo::xms::SignalSlotable::DataHandler& dataHandler,
                                        const karabo::data::Hash& inputChannelCfg = karabo::data::Hash(),
                                        const karabo::xms::SignalSlotable::InputHandler& eosHandler =
                                              karabo::xms::SignalSlotable::InputHandler(),
                                        const karabo::xms::SignalSlotable::InputHandler& inputHandler =
                                              karabo::xms::SignalSlotable::InputHandler());

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
             * Set a remote property in the distributed system
             * @param instanceId of the device to set the property on
             * @param key path to the property to set
             * @param value to set
             * @param timeoutInSeconds maximum timeout until set operation fails, set to -1 to wait forever
             * @param keySep path separator
             */
            template <class T>
            void set(const std::string& instanceId, const std::string& key, const T& value, int timeoutInSeconds = -1,
                     const char keySep = data::Hash::k_defaultSep) {
                karabo::data::Hash tmp;
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
            void setNoWait(const std::string& instanceId, const std::string& key, const T& value,
                           const char keySep = data::Hash::k_defaultSep) {
                karabo::data::Hash tmp;
                tmp.set(key, value, keySep);
                setNoWait(instanceId, tmp);
            }

            /**
             * Bulk-set remote properties in the distributed system
             * @param instanceId of the device to set the property on
             * @param values a Hash containing the to be set value in a path structure indicating which properties to
             * set
             * @param timeoutInSeconds maximum timeout until set operation fails, set to -1 to wait forever
             */
            void set(const std::string& instanceId, const karabo::data::Hash& values, int timeoutInSeconds = -1);

            /**
             * Bulk-set remote properties in the distributed system as a fire-and-forget operation.
             * Warning: there is no guarantee and indication if the set succeeded!
             * @param instanceId of the device to set the property on
             * @param values a Hash containing the to be set value in a path structure indicating which properties to
             * set
             */
            void setNoWait(const std::string& instanceId, const karabo::data::Hash& values);

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
             * @brief Executes a function on a device synchronously (waits until the function finished)
             * @tparam Args Variadic template for the slot args (no arg is a particular case).
             * @param deviceId The devideId
             * @param command The command
             * @param timeoutInSeconds Timeout
             */
            template <typename... Args>
            void execute(const std::string& deviceId, const std::string& command, int timeoutInSeconds = 3,
                         const Args&... slotArgs) {
                // For supporting legacy code that uses -1 as the default timeout value.
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;
                KARABO_GET_SHARED_FROM_WEAK(sp, m_signalSlotable);
                sp->request(deviceId, command, slotArgs...).timeout(timeoutInSeconds * 1000).receive();
            }

            /**
             * @brief Synchronously executes a slot that returns a single element response.
             *
             * @tparam R1 Type of the response.
             * @tparam Args Variadic template for the slot arguments.
             * @param deviceId Id of the device whose slot should be executed.
             * @param slotName Name of the slot to execute.
             * @param timeoutInSeconds Timeout for the slot execution.
             * @param slotArgs Slot arguments.
             * @return A value of R1 type
             */
            template <typename R1, typename... Args>
            R1 execute1(const std::string& deviceId, const std::string& slotName, int timeoutInSeconds = 3,
                        const Args&... slotArgs) {
                // For backwards compatibility with BoundApi.
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;

                KARABO_GET_SHARED_FROM_WEAK(sp, m_signalSlotable);
                // Note: karabo::util::unpack is the workhorse that performs the transformation of the hash body
                //       of the request's response into the variadic list of arguments passed to receive.
                R1 resp;
                sp->request(deviceId, slotName, slotArgs...).timeout(timeoutInSeconds * 1000).receive(resp);
                return resp;
            }

            /**
             * @brief Synchronously executes a slot that returns a two element
             *        tuple as a response.
             *
             * @note a tuple, instead of a pair, is used as the return value
             *       for uniformity with the other executeN methods.
             *
             * @tparam R1 Type of first element of the resulting pair.
             * @tparam R2 Type of second element of the resulting pair.
             * @tparam Args Variadic template for the slot arguments.
             * @param deviceId Id of the device whose slot should be executed.
             * @param slotName Name of the slot to execute.
             * @param timeoutInSeconds Timeout for the slot execution.
             * @param slotArgs Slot arguments.
             * @return std::tuple<R1, R2> with the results of the slot execution.
             */
            template <typename R1, typename R2, typename... Args>
            std::tuple<R1, R2> execute2(const std::string& deviceId, const std::string& slotName,
                                        int timeoutInSeconds = 3, const Args&... slotArgs) {
                // For backwards compatibility with BoundApi.
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;

                KARABO_GET_SHARED_FROM_WEAK(sp, m_signalSlotable);
                R1 st;
                R2 nd;
                sp->request(deviceId, slotName, slotArgs...).timeout(timeoutInSeconds * 1000).receive(st, nd);
                auto res = std::make_tuple<R1, R2>(std::move(st), std::move(nd));
                return res;
            }

            /**
             * @brief Synchronously executes a slot that returns a three element tuple as a
             *        response.
             *
             * @tparam R1 Type of first element of the resulting tuple.
             * @tparam R2 Type of second element of the resulting tuple.
             * @tparam R3 Type of third element of the resulting tuple.
             * @tparam Args Variadic template for the slot arguments.
             * @param deviceId Id of the device whose slot should be executed.
             * @param slotName Name of the slot to execute.
             * @param timeoutInSeconds Timeout for the slot execution.
             * @param slotArgs Slot arguments.
             * @return std::tuple<R1, R2, R3> Tuple with the results of the
             * slot execution.
             */
            template <typename R1, typename R2, typename R3, typename... Args>
            std::tuple<R1, R2, R3> execute3(const std::string& deviceId, const std::string& slotName,
                                            int timeoutInSeconds = 3, const Args&... slotArgs) {
                // For backwards compatibility with BoundApi.
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;

                KARABO_GET_SHARED_FROM_WEAK(sp, m_signalSlotable);
                R1 st;
                R2 nd;
                R3 rd;
                sp->request(deviceId, slotName, slotArgs...).timeout(timeoutInSeconds * 1000).receive(st, nd, rd);
                auto res = std::make_tuple<R1, R2, R3>(std::move(st), std::move(nd), std::move(rd));
                return res;
            }


            /**
             * @brief Synchronously executes a slot that returns a four element tuple as a
             *        response.
             *
             * @tparam R1 Type of first element of the resulting tuple.
             * @tparam R2 Type of second element of the resulting tuple.
             * @tparam R3 Type of third element of the resulting tuple.
             * @tparam R4 Type of fourth element of the resulting tuple.
             * @tparam Args Variadic template for the slot arguments.
             * @param deviceId Id of the device whose slot should be executed.
             * @param slotName Name of the slot to execute.
             * @param timeoutInSeconds Timeout for the slot execution.
             * @param slotArgs Slot arguments.
             * @return std::tuple<R1, R2, R3, R4> Tuple with the results of the
             * slot execution.
             */
            template <typename R1, typename R2, typename R3, typename R4, typename... Args>
            std::tuple<R1, R2, R3, R4> execute4(const std::string& deviceId, const std::string& slotName,
                                                int timeoutInSeconds = 3, const Args&... slotArgs) {
                KARABO_GET_SHARED_FROM_WEAK(sp, m_signalSlotable);
                R1 st;
                R2 nd;
                R3 rd;
                R4 th;
                sp->request(deviceId, slotName, slotArgs...).timeout(timeoutInSeconds * 1000).receive(st, nd, rd, th);
                auto res = std::make_tuple<R1, R2, R3, R4>(std::move(st), std::move(nd), std::move(rd), std::move(th));
                return res;
            }

            /**
             * Request the data schema for an output channel as a Hash containing relevant information
             * @param deviceId
             * @param outputChannelName
             * @return a Hash containing the output channel's data schema
             */
            karabo::data::Hash getOutputChannelSchema(const std::string& deviceId,
                                                      const std::string& outputChannelName);

            /**
             * Get the list of all output channel names of the remote device.
             * @param deviceId
             * @return vector containing output channel names
             */
            std::vector<std::string> getOutputChannelNames(const std::string& deviceId);

           protected: // functions
            void initTopology();

            void cacheAvailableInstances();

            /**
             * Prepare a topology entry for the runtime system description
             * @param path   the path created with <i>prepareTopologyPath</i> using instanceId and instanceInfo
             * @param instanceInfo   The instanceInfo Hash received from the broadcast
             */
            karabo::data::Hash prepareTopologyEntry(const std::string& path,
                                                    const karabo::data::Hash& instanceInfo) const;

            std::string prepareTopologyPath(const std::string& instanceId,
                                            const karabo::data::Hash& instanceInfo) const;

            void removeFromSystemTopology(const std::string& instanceId);

            virtual void setupSlots();

            virtual void _slotChanged(const karabo::data::Hash& hash, const std::string& instanceId);

            void _slotInstanceNew(const std::string& instanceId, const karabo::data::Hash& instanceInfo);

            void _slotInstanceUpdated(const std::string& instanceId, const karabo::data::Hash& instanceInfo);

            void _slotInstanceGone(const std::string& instanceId, const karabo::data::Hash& instanceInfo);

            virtual void _slotSchemaUpdated(const karabo::data::Schema& schema, const std::string& deviceId);

            virtual void _slotClassSchema(const karabo::data::Schema& schema, const std::string& classId,
                                          const std::string& serverId);

            virtual void _slotLoggerMap(const karabo::data::Hash& loggerMap);

            static std::string generateOwnInstanceId();

            karabo::data::Schema cacheAndGetClassSchema(const std::string& serverId, const std::string& classId);

            karabo::data::Schema cacheAndGetDeviceSchema(const std::string& instanceId);

            karabo::data::Schema cacheAndGetActiveSchema(const std::string& instanceId);

            karabo::data::Hash cacheAndGetConfiguration(const std::string& instanceId);

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
                               const std::function<void()>& asyncSuccessHandler = std::function<void()>(),
                               const std::function<void()>& asyncFailureHandler = std::function<void()>());

            void eraseFromInstanceUsage(const std::string& instanceId);

            virtual void notifyDeviceChangedMonitors(const karabo::data::Hash& hash, const std::string& instanceId);

            virtual void notifyPropertyChangedMonitors(const karabo::data::Hash& hash, const std::string& instanceId);

            void castAndCall(const std::string& instanceId, const karabo::data::Hash& registered,
                             const karabo::data::Hash& current, std::string path = "") const;

            void extractCommands(const karabo::data::Schema& schema, const std::string& parentKey,
                                 std::vector<std::string>& commands);

            std::vector<std::string> filterProperties(const karabo::data::Schema& schema, const int accessLevel);

            std::string getInstanceType(const karabo::data::Hash& instanceInfo) const;

            virtual void slotProvideSystemTopology();

            void age(const boost::system::error_code& e);

            void disconnect(const std::string& instanceId);

            void disconnectHandler(const std::string& signal, const std::string& instanceId,
                                   const std::vector<std::string>& toClear);

            void sendSignalsChanged(const boost::system::error_code& e);

            void kickSignalsChangedTimer();

            void immortalize(const std::string& deviceId);

            /// Unmark deviceId from staying connected all the time without ageing.
            ///
            /// Also clears a zombie (marked by negative age) from m_instanceUsage and thus locks m_instanceUsageMutex.
            /// That means, unlike immortalize(..) and isImortal(..), mortalize(..) must not be called under protection
            /// of m_instanceUsageMutex.
            void mortalize(const std::string& deviceId);

            bool isImmortal(const std::string& deviceId) const;

            void mergeIntoRuntimeSystemDescription(const karabo::data::Hash& entry);

            bool existsInRuntimeSystemDescription(const std::string& path) const;

            /// returns true if path could be removed
            bool eraseFromRuntimeSystemDescription(const std::string& path);

            /// Get section (e.g. "device") from runtime description.
            /// Returns empty Hash if section does not exist.
            data::Hash getSectionFromRuntimeDescription(const std::string& section) const;

            /// Find full path of 'instanceId' in m_runtimeSystemDescription,
            /// empty if path does not exist.
            std::string findInstanceSafe(const std::string& instanceId) const;

           private:
            /// As findInstanceSafe, but to be called under protection of m_runtimeSystemDescriptionMutex.
            std::string findInstance(const std::string& instanceId) const;

            /// Actually process data in 'signalChangedMap' - try/catch should be outside.
            void doSendSignalsChanged(const SignalChangedMap& signalChangedMap);

            /// Marks 'instanceId' as used.
            /// Returns true if explicit "connect" call should still be done for it.
            bool connectNeeded(const std::string& instanceId);

            void connectAndRequest(const std::string& deviceId);

            void completeInitialization(int countdown);

            /**
             * @brief Internal helper method to initialize the service device ids members of
             * the DeviceClient instance.
             *
             * @param serviceDeviceIds A hash with ids of core service devices; e.g, "dataLoggerManagerId" key and the
             *                         value is the name of the DataLoggerManager the device client instance should use
             *                         for data logging operations. Currently keys "dataLoggerManagerId" and
             *                         "configurationManagerId" are supported. If a supported key is missing, the
             *                         default ID for the service device type is used.
             */
            void initServiceDeviceIds(const karabo::data::Hash& serviceDeviceIds);

            /**
             * Helper for _slotInstanceGone for servers
             *
             * Finds all devices that belong to given server, removes them from  m_runtimeSystemDescription and returns
             * pairs of their deviceIds and instanceInfo.
             * Requires protection of m_runtimeSystemDescriptionMutex.
             */
            std::vector<std::pair<std::string, karabo::data::Hash>> findAndEraseDevicesAsGone(
                  const std::string& serverId);

            /**
             * Helper for _slotInstanceGone
             *
             * Does all needed action
             * - despite of removal from m_runtimeSystemDescription
             * - and despite of special treatment of devices on the server if instance is a server
             */
            void treatInstanceAsGone(const std::string& instanceId, const karabo::data::Hash& instanceInfo);
        };
    } // namespace core
} // namespace karabo


#endif
