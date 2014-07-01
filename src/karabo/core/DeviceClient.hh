/*
 * $Id: Com.hh 6624 2012-06-27 12:57:09Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/util/Timestamp.hh>

#ifndef KARABO_CORE_DEVICE_CLIENT_HH
#define	KARABO_CORE_DEVICE_CLIENT_HH

namespace karabo {

    namespace core {

        // Forward
        template <class T>
        class Device;

        /**
         * The Karabo Device Client
         * This class can be used to (remotely) control devices of the distributed system
         * Synchronous calls (i.e. get()) are in fact asynchronous under the hood
         * 
         * Following signals and slots are available on the various components:
         * 
         * All instances:
         * 
         * ## SIGNALS ##
         *   1) signalInstanceUpdated(string, Hash) // instanceId, instanceInfo
         *      Implemented in SignalSlotable and emitted if new instance is started.
         *      Re-emitted whenever instanceInfo changes.
         *  
         *   2) signalInstanceGone(string) // instanceId
         *      Implemented in SignalSlotable and emitted if instance is going down.
         * 
         *   NOTE: Both signals are connected to corresponding global slots
         * 
         * ## SLOTS ##
         *   1) slotStopEventLoop()
         *      Calls stop() on the IOService object, will unblock the run() method and stop communicating  
         * Device:
         *   1) signalChanged(Hash, string) // Changed configuration, deviceId
         *   2) 
         * 
         * 
         */
        class DeviceClient {

            template<class T>
            friend class Device;

            typedef std::map<std::string, unsigned int> InstanceUsage;
            typedef boost::function<void (const karabo::util::Hash& /*topologyEntry*/) > InstanceNewHandler;
            typedef boost::function<void (const karabo::util::Hash& /*topologyEntry*/) > InstanceUpdatedHandler;
            typedef boost::function<void (const std::string& /*instanceId*/, const karabo::util::Hash& /*instanceInfo*/) > InstanceGoneHandler;
            typedef boost::function<void (const std::string& /*deviceId*/, const karabo::util::Schema& /*schema*/, const karabo::util::Hash& /*configuration*/) > SchemaUpdatedHandler;
            typedef boost::function<void (const std::string& /*serverId*/, const std::string& /*classId*/, const karabo::util::Schema& /*schema*/) > ClassSchemaHandler;

            static const unsigned int CONNECTION_KEEP_ALIVE = 15;

        protected: // members

            enum MasterMode {

                IS_MASTER,
                HAS_MASTER,
                NO_MASTER
            };

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

            boost::shared_ptr<karabo::xms::SignalSlotable> m_signalSlotable;

            bool m_isShared;

            boost::thread m_eventThread;

            InstanceUsage m_instanceUsage;

            karabo::util::Hash m_deviceChangedHandlers;

            karabo::util::Hash m_propertyChangedHandlers;

            boost::mutex m_instanceUsageMutex;

            boost::mutex m_deviceChangedHandlersMutex;

            boost::mutex m_propertyChangedHandlersMutex;

            int m_internalTimeout;

            std::string m_masterDeviceId;

            bool m_isAdvancedMode; // DEPRECATED

            MasterMode m_masterMode;

            boost::thread m_ageingThread;

            bool m_getOlder;

            InstanceNewHandler m_instanceNewHandler;
            InstanceUpdatedHandler m_instanceUpdatedHandler;
            InstanceGoneHandler m_instanceGoneHandler;
            SchemaUpdatedHandler m_schemaUpdatedHandler;
            ClassSchemaHandler m_classSchemaHandler;

            std::set<std::string> m_immortals;
            mutable boost::mutex m_immortalsMutex;

        public:

            KARABO_CLASSINFO(DeviceClient, "DeviceClient", "1.0");

            /**
             * Constructor which establishes an own connection to the communication system (default JMS - OpenMQ)
             * This constructor is intended for stand-alone C++ device clients (and thus needs authentication)
             * @param connectionType The communication system transport layer implementation
             * @param connectionParameters Additional connection configuration
             */
            DeviceClient(const std::string& connectionType = "Jms",
                         const karabo::util::Hash& connectionParameters = karabo::util::Hash());

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

            // DEPRECATE
            void enableAdvancedMode();

            // DEPRECATE
            void disableAdvancedMode();

            /**
             * Set ageing on or off (on by default)
             * @return 
             */
            void setAgeing(bool toggle);



            /**
             * Allows asking whether an instance is online in the current distributed system
             * @param boolean indicating whether existing and hostname if exists
             * @return 
             */
            std::pair<bool, std::string> exists(const std::string& instanceId);

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

            std::vector<std::string> getDevices(const std::string& deviceServer);

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

            karabo::util::Schema getClassSchemaNoWait(const std::string& serverId, const std::string& classId);

            std::vector<std::string> getProperties(const std::string& deviceId);

            std::vector<std::string> getClassProperties(const std::string& serverId, const std::string& classId);

            std::vector<std::string> getCurrentlyExecutableCommands(const std::string& instanceId);

            std::vector<std::string> getCurrentlySettableProperties(const std::string& instanceId);

            //karabo::util::Hash loadConfigurationFromDB(const std::string& configurationId);

            karabo::util::Hash loadConfigurationFromFile(const std::string& filename);

            std::pair<bool, std::string> instantiate(const std::string& serverInstanceId, const std::string& classId,
                                                     const karabo::util::Hash& configuration = karabo::util::Hash(),
                                                     int timeoutInSeconds = -1);

            std::pair<bool, std::string> instantiate(const std::string& serverInstanceId,
                                                     const karabo::util::Hash& configuration,
                                                     int timeoutInSeconds = -1);

            void instantiateNoWait(const std::string& serverInstanceId, const std::string& classId,
                                   const karabo::util::Hash& configuration = karabo::util::Hash());

            void instantiateNoWait(const std::string& serverInstanceId, const karabo::util::Hash& configuration);

            std::pair<bool, std::string> killDevice(const std::string& deviceId, int timeoutInSeconds = -1);

            void killDeviceNoWait(const std::string& deviceId);

            std::pair<bool, std::string> killServer(const std::string& serverId, int timeoutInSeconds = -1);

            void killServerNoWait(const std::string& serverId);

            karabo::util::Hash get(const std::string& instanceId);

            void get(const std::string& instanceId, karabo::util::Hash& hash);

            karabo::util::Hash getConfigurationNoWait(const std::string& deviceId);

            template<class T>
            T get(const std::string& instanceId, const std::string& key, const char keySep = '.') {
                try {
                    return cacheAndGetConfiguration(instanceId).get<T > (key, keySep);
                } catch (const karabo::util::Exception& e) {

                    throw KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key + "\" from device \"" + instanceId + "\"");
                }
            }

            template<class T>
            void get(const std::string& instanceId, const std::string& key, T& value, const char keySep = '.') {
                try {
                    return cacheAndGetConfiguration(instanceId).get(key, value, keySep);
                } catch (const karabo::util::Exception& e) {

                    throw KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key + "\" from device \"" + instanceId + "\"");
                }
            }

            template<class T>
            T getAs(const std::string& instanceId, const std::string& key, const char keySep = '.') {
                try {
                    return cacheAndGetConfiguration(instanceId).getAs<T > (key, keySep);
                } catch (const karabo::util::Exception& e) {

                    throw KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key + "\" from device \"" + instanceId + "\"");
                }
            }

            karabo::util::vector<karabo::util::Hash> getFromPast(const std::string& deviceId, const std::string& key, const std::string& from, std::string to = "", unsigned int maxNumData = 0);

            karabo::util::vector<karabo::util::Hash> getPropertyHistory(const std::string& deviceId, const std::string& key, const std::string& from, std::string to = "", unsigned int maxNumData = 0);

            std::pair<karabo::util::Hash, karabo::util::Schema> getConfigurationFromPast(const std::string& deviceId, const std::string& timepoint);

            void registerInstanceNewMonitor(const InstanceNewHandler& callBackFunction);

            void registerInstanceUpdatedMonitor(const InstanceUpdatedHandler& callBackFunction);

            void registerInstanceGoneMonitor(const InstanceGoneHandler& callBackFunction);

            void registerSchemaUpdatedMonitor(const SchemaUpdatedHandler& callBackFunction);

            void registerClassSchemaMonitor(const ClassSchemaHandler& callBackFunction);

            template <class ValueType>
            bool registerPropertyMonitor(const std::string& instanceId, const std::string& key,
                                         const boost::function<void (const std::string& /*deviceId*/, const std::string& /*key*/, const ValueType& /*value*/, const karabo::util::Timestamp& /*timestamp*/) >& callbackFunction) {
                karabo::util::Schema schema = this->getDeviceSchema(instanceId);
                if (schema.has(key)) {
                    boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                    this->cacheAndGetConfiguration(instanceId);
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._function", callbackFunction);
                    immortalize(instanceId);
                    return true;
                } else {
                    return false;
                }
            }

            // TODO provide timestamp in callback

            template <class ValueType, class UserDataType>
            bool registerPropertyMonitor(const std::string& instanceId, const std::string& key, const boost::function<void ( const std::string& /*deviceId*/, const std::string& /*key*/,
                                         const ValueType& /*value*/, const karabo::util::Timestamp& /*timestamp*/, const boost::any& /*userData*/) >& callbackFunction, const UserDataType& userData) {
                karabo::util::Schema schema = this->getDeviceSchema(instanceId);
                if (schema.has(key)) {
                    boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                    this->cacheAndGetConfiguration(instanceId);
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._function", callbackFunction);
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._userData", userData);
                    immortalize(instanceId);
                    return true;
                } else {
                    return false;
                }
            }

            void unregisterPropertyMonitor(const std::string& instanceId, const std::string& key);

            void registerDeviceMonitor(const std::string& instanceId, const boost::function<void (const std::string&, const karabo::util::Hash&)>& callbackFunction);

            // TODO Adapt function to above style (i.e. add return value)

            template <class UserDataType>
            void registerDeviceMonitor(const std::string& instanceId, const boost::function<void (const std::string&, const karabo::util::Hash&, const boost::any&)>& callbackFunction,
                                       const UserDataType& userData) {

                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                // Make sure we are caching this instanceId
                this->cacheAndGetConfiguration(instanceId);
                m_deviceChangedHandlers.set(instanceId + "._function", callbackFunction);
                m_deviceChangedHandlers.set(instanceId + "._userData", userData);
                immortalize(instanceId);
            }

            void unregisterDeviceMonitor(const std::string& instanceId);

            template <class T>
            std::pair<bool, std::string> set(const std::string& instanceId, const std::string& key, const T& value, int timeoutInSeconds = -1, const char keySep = '.') {

                karabo::util::Hash tmp;
                tmp.set(key, value, keySep);
                return set(instanceId, tmp, timeoutInSeconds);
            }

            template <class T>
            void setNoWait(const std::string& instanceId, const std::string& key, const T& value, const char keySep = '.') {

                karabo::util::Hash tmp;
                tmp.set(key, value, keySep);
                setNoWait(instanceId, tmp);
            }

            std::pair<bool, std::string> set(const std::string& instanceId, const karabo::util::Hash& values, int timeoutInSeconds = -1);

            void setNoWait(const std::string& instanceId, const karabo::util::Hash& values);

            void executeNoWait(const std::string& instanceId, const std::string& command) {

                m_signalSlotable->call(instanceId, command);
            }

            template <class A1>
            void executeNoWait(const std::string& instanceId, const std::string& command, const A1& a1) {

                m_signalSlotable->call(instanceId, command, a1);
            }

            template <class A1, class A2>
            void executeNoWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2) {

                m_signalSlotable->call(instanceId, command, a1, a2);
            }

            template <class A1, class A2, class A3>
            void executeNoWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, const A3& a3) {

                m_signalSlotable->call(instanceId, command, a1, a2, a3);
            }

            template <class A1, class A2, class A3, class A4>
            void executeNoWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {

                m_signalSlotable->call(instanceId, command, a1, a2, a3, a4);
            }

            std::pair<bool, std::string> execute(const std::string& instanceId, const std::string& command, int timeoutInSeconds = -1) {
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command).timeout(timeoutInSeconds * 1000).receive(text);
                } catch (const karabo::util::Exception& e) {

                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return std::make_pair(ok, text);
            }

            template <class A1>
            std::pair<bool, std::string> execute(const std::string& instanceId, const std::string& command, const A1& a1, int timeoutInSeconds = -1) {
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1).timeout(timeoutInSeconds * 1000).receive(text);
                } catch (const karabo::util::Exception& e) {

                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return std::make_pair(ok, text);
            }

            template <class A1, class A2>
            std::pair<bool, std::string> execute(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, int timeoutInSeconds = -1) {
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1, a2).timeout(timeoutInSeconds * 1000).receive(text);
                } catch (const karabo::util::Exception& e) {

                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return std::make_pair(ok, text);
            }

            template <class A1, class A2, class A3>
            std::pair<bool, std::string> execute(const std::string& instanceId, const std::string& command,
                                                 const A1& a1, const A2& a2, const A3& a3, int timeoutInSeconds = -1) {
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1, a2, a3).timeout(timeoutInSeconds * 1000).receive(text);
                } catch (const karabo::util::Exception& e) {

                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return std::make_pair(ok, text);
            }

            template <class A1, class A2, class A3, class A4>
            std::pair<bool, std::string> execute(const std::string& instanceId, const std::string& command,
                                                 const A1& a1, const A2& a2, const A3& a3, const A4& a4, int timeoutInSeconds = -1) {
                if (timeoutInSeconds == -1) timeoutInSeconds = 3;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1, a2, a3, a4).timeout(timeoutInSeconds * 1000).receive(text);
                } catch (const karabo::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return std::make_pair(ok, text);
            }

        protected: // functions

            void cacheAvailableInstances();

            karabo::util::Hash prepareTopologyEntry(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            std::string prepareTopologyPath(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void removeFromSystemTopology(const std::string& instanceId);

            virtual void setupSlots();

            virtual void slotChanged(const karabo::util::Hash& hash, const std::string& instanceId);

            virtual void slotInstanceNew(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            virtual void slotInstanceUpdated(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            virtual void slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            virtual void slotSchemaUpdated(const karabo::util::Schema& schema, const karabo::util::Hash& configuration, const std::string& deviceId);

            virtual void slotClassSchema(const karabo::util::Schema& schema, const std::string& classId, const std::string& serverId);



            void onInstanceNotAvailable(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void onInstanceAvailableAgain(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            static std::string generateOwnInstanceId();

            karabo::util::Schema cacheAndGetClassSchema(const std::string& serverId, const std::string& classId);

            karabo::util::Schema cacheAndGetDeviceSchema(const std::string& instanceId);

            karabo::util::Schema cacheAndGetActiveSchema(const std::string& instanceId);

            karabo::util::Hash cacheAndGetConfiguration(const std::string& instanceId);

            void stayConnected(const std::string& instanceId);

            void disconnect(const std::string& instanceId);

            virtual void notifyDeviceChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId);

            virtual void notifyPropertyChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId);

            void castAndCall(const std::string& instanceId, const karabo::util::Hash& registered, const karabo::util::Hash& current, std::string path = "") const;

            void extractCommands(const karabo::util::Schema& schema, const std::string& parentKey, std::vector<std::string>& commands);

            std::vector<std::string> filterProperties(const karabo::util::Schema& schema, const int accessLevel);

            void checkMaster();

            std::string getInstanceType(const karabo::util::Hash& instanceInfo) const;

            virtual void slotMasterPing();

            virtual void slotProvideSystemTopology();

            void age();

            void immortalize(const std::string& deviceId);

            void mortalize(const std::string& deviceId);

            bool isImmortal(const std::string& deviceId) const;

            void mergeIntoRuntimeSystemDescription(const karabo::util::Hash& entry);

            bool existsInRuntimeSystemDescription(const std::string& path) const;

            void eraseFromRuntimeSystemDescription(const std::string& path);

        };
    }
}


#endif

