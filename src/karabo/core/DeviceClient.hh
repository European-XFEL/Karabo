/*
 * $Id: Com.hh 6624 2012-06-27 12:57:09Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/xms/SignalSlotable.hh>

#ifndef KARABO_CORE_DEVICE_CLIENT_HH
#define	KARABO_CORE_DEVICE_CLIENT_HH

namespace karabo {

    namespace core {

        /**
         * The Karabo Device Client
         * This class can be used to (remotely) control devices of the distributed system
         * Synchronous calls (i.e. get()) are in fact asynchronous under the hood
         */
        class DeviceClient {

            typedef std::map<std::string, karabo::util::Schema> FullSchemaCache;

            typedef std::map<std::string, std::map< std::string, karabo::util::Schema > > CurrentStateSchemaCache;

            typedef std::map<std::string, karabo::util::Hash> ConfigurationCache;

            typedef std::vector<karabo::util::Hash> DbTableCache;

            typedef std::map<std::string, unsigned int> InstanceUsage;



            /**
             * #servers +
             *   serverName \host(STRING) +
             *     className +
             *       #description SCHEMA
             *       #configuration HASH
             *       #instances VECTOR_STRING        
             * #devices +
             *   deviceName \serverName(STRING) \className(STRING)
             *     #fullSchema SCHEMA
             *     #activeSchema +
             *         stateName +
             *           roleName SCHEMA
             *     #configuration HASH
             */
            karabo::util::Hash m_runtimeSystemDescription;
            
            boost::mutex m_runtimeSystemDescriptionMutex;
           

            std::string m_role;

        public:

            KARABO_CLASSINFO(DeviceClient, "DeviceClient", "1.0")


            DeviceClient(const std::string& connectionType = "Jms", const karabo::util::Hash& connectionParameters = karabo::util::Hash());

            /**
             * Constructor using instantiated signalSlotable class (shared communication)
             * @param signalSlotable An instance of the SignalSlotable lass
             */
            DeviceClient(const boost::shared_ptr<karabo::xms::SignalSlotable>& signalSlotable);

            virtual ~DeviceClient();

            /**
             * Sets the default timeout for any request/response like communications
             * @param defaultTimeout The default timeout in ms
             */
            void setDefaultTimeout(const unsigned int defaultTimeout);

            /**
             * Retrieves the currently set default timeout
             * @return default timeout in ms
             */
            int getDefaultTimeout() const;

            void setDefaultKeySeparator(const std::string& defaultKeySep);

            const std::string& getDefaultKeySeparator() const;

            /**
             * Allows asking whether an instance is online in the current distributed system
             * @param boolean indicating whether existing and hostname if exists
             * @return 
             */
            std::pair<bool, std::string> exists(const std::string& instanceId);

            
            std::vector<std::string> getDeviceServers();
            
            std::vector<std::string> getDeviceClasses(const std::string& deviceServer);

            std::vector<std::string> getDevices();
            
            /**
             * Retrieves the full Schema (parameter description) of the given instance;
             * @param instanceId Device's instance ID
             * @return full Schema
             */
            const karabo::util::Schema& getFullSchema(const std::string& instanceId);

            /**
             * Retrieves the currently active Schema (filtered by allowed states and allowed roles)
             * of the given instance
             * @param instanceId Device's instance ID
             * @return active Schema
             */
            const karabo::util::Schema& getActiveSchema(const std::string& instanceId);

            /**
             * Retrieves a schema from static context of a loaded Device class plug-in.
             * This schema represents a description of parameters possible to configure for instantiation.
             * I.e. returns in fact a description of the constructor arguments to that device class.
             * @param deviceServer instanceId of a deviceServer
             * @param className name of loaded class on the deviceServer (classId)
             * @return Schema describing parameters available at instantiation time
             */
            const karabo::util::Schema& getClassSchema(const std::string& deviceServer, const std::string& className);

            std::vector<std::string> getDeviceParameters(const std::string& instanceId, const std::string& key = "", const std::string& keySep = "");

            std::vector<std::string> getDeviceParametersFlat(const std::string& instanceId, const std::string& keySep = "");

            std::vector<std::string> getCurrentlyExecutableCommands(const std::string& instanceId, const std::string& keySep = "");

            std::vector<std::string> getCurrentlySettableProperties(const std::string& instanceId, const std::string& keySep = "");


            karabo::util::Hash loadConfigurationFromDB(const std::string& configurationId);

            karabo::util::Hash loadConfigurationFromFile(const std::string& filename);

            void instantiateNoWait(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash& configuration = karabo::util::Hash());

            void instantiateNoWait(const std::string& serverInstanceId, const karabo::util::Hash& configuration);

            std::pair<bool, std::string> instantiateWait(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash& configuration = karabo::util::Hash(), int timeout = -1);

            std::pair<bool, std::string> instantiateWait(const std::string& serverInstanceId, const karabo::util::Hash& configuration, int timeout = -1);

            void kill(const std::string& instanceId);

            template<class T>
            T get(const std::string& instanceId, const std::string& key, const std::string& keySep = ".") {
                return cacheAndGetConfiguration(instanceId).get<T > (key, keySep);
            }

            template<class T>
            void get(const std::string& instanceId, const std::string& key, T& value, const std::string& keySep = ".") {
                return cacheAndGetConfiguration(instanceId).get(key, value);
            }

            const karabo::util::Hash& get(const std::string& instanceId);

            void get(const std::string& instanceId, karabo::util::Hash& hash);

            template <class ValueType>
            bool registerMonitor(const std::string& instanceId, const std::string& key, const boost::function<void (const ValueType&, const std::string&) >& callbackFunction) {
                karabo::util::Schema schema = this->getFullSchema(instanceId);
                if (schema.has(key)) {
                    boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                    this->cacheAndGetConfiguration(instanceId);
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._function", callbackFunction);
                    return true;
                } else {
                    return false;
                }
            }

            template <class ValueType, class UserDataType>
            bool registerMonitor(const std::string& instanceId, const std::string& key, const boost::function<void (const ValueType&, const std::string&, const boost::any&) >& callbackFunction, const UserDataType& userData) {
                karabo::util::Schema schema = this->getFullSchema(instanceId);
                if (schema.has(key)) {
                    boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                    this->cacheAndGetConfiguration(instanceId);
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._function", callbackFunction);
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._userData", userData);
                    return true;
                } else {
                    return false;
                }
            }

            void unregisterMonitor(const std::string& instanceId, const std::string& key);

            void registerMonitor(const std::string& instanceId, const boost::function<void (const karabo::util::Hash&, const std::string&)>& callbackFunction);

            template <class UserDataType>
            void registerMonitor(const std::string& instanceId, const boost::function<void (const karabo::util::Hash&, const std::string&, const boost::any&)>& callbackFunction, const UserDataType& userData) {
                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                // Make sure we are caching this instanceId
                this->cacheAndGetConfiguration(instanceId);
                m_deviceChangedHandlers.set(instanceId + "._function", callbackFunction);
                m_deviceChangedHandlers.set(instanceId + "._userData", userData);
            }

            void unregisterMonitor(const std::string& instanceId);

            template <class T>
            std::pair<bool, std::string> setWait(const std::string& instanceId, const std::string& key, const T& value, const std::string& keySep = ".", int timeout = -1) const {
                karabo::util::Hash tmp;
                tmp.set(key, value, keySep);
                return setWait(instanceId, tmp, timeout);
            }

            template <class T>
            void setNoWait(const std::string& instanceId, const std::string& key, const T& value, const std::string& keySep = ".") const {
                karabo::util::Hash tmp;
                tmp.set(key, value, keySep);
                setNoWait(instanceId, tmp);
            }

            std::pair<bool, std::string> setWait(const std::string& instanceId, const karabo::util::Hash& values, int timeout = -1) const;

            void setNoWait(const std::string& instanceId, const karabo::util::Hash& values) const;

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

            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, int timeout = -1) {
                if (timeout == -1) timeout = m_defaultTimeout;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command).timeout(timeout).receive(text);
                } catch (const karabo::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return std::make_pair(ok, text);
            }

            template <class A1>
            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, int timeout = -1) {
                if (timeout == -1) timeout = m_defaultTimeout;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1).timeout(timeout).receive(text);
                } catch (const karabo::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return std::make_pair(ok, text);
            }

            template <class A1, class A2>
            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, int timeout = -1) {
                if (timeout == -1) timeout = m_defaultTimeout;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1, a2).timeout(timeout).receive(text);
                } catch (const karabo::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return std::make_pair(ok, text);
            }

            template <class A1, class A2, class A3>
            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, const A3& a3, int timeout = -1) {
                if (timeout == -1) timeout = m_defaultTimeout;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1, a2, a3).timeout(timeout).receive(text);
                } catch (const karabo::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return std::make_pair(ok, text);
            }

            template <class A1, class A2, class A3, class A4>
            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, const A3& a3, const A4& a4, int timeout = -1) {
                if (timeout == -1) timeout = m_defaultTimeout;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1, a2, a3, a4).timeout(timeout).receive(text);
                } catch (const karabo::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return std::make_pair(ok, text);
            }

        protected: // functions
            
            void setupSlots();

            static std::string generateOwnInstanceId();

            const karabo::util::Schema& getCurrentlyWritableSchemaForParameter(const std::string& instanceId, const std::string& key, const std::string& keySep);
            
            void cacheAvailableInstances();

            karabo::util::Schema& cacheAndGetFullSchema(const std::string& instanceId);

            karabo::util::Schema& cacheAndGetActiveSchema(const std::string& instanceId);

            const karabo::util::Hash& cacheAndGetConfiguration(const std::string& instanceId);

            const DbTableCache& cacheAndGetDeviceServers();

            const DbTableCache& cacheAndGetDeviceClasses();

            const DbTableCache& cacheAndGetDevices();



            void refreshInstanceUsage(const std::string& instanceId);

            virtual void slotChanged(const karabo::util::Hash& hash, const std::string& instanceId);

            virtual void slotNewDeviceServerInstance(const karabo::util::Hash& hash);

            virtual void slotUpdateDeviceServerInstance(const karabo::util::Hash& hash);

            virtual void slotNewDeviceInstance(const karabo::util::Hash& hash);

            virtual void slotUpdateDeviceInstance(const karabo::util::Hash& hash);

            virtual void notifyDeviceChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId);

            virtual void notifyPropertyChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId);

            void castAndCall(const std::string& instanceId, const karabo::util::Hash& registered, const karabo::util::Hash& current, std::string path = "") const;

            virtual void clearCacheAndDisconnect(const std::string& instanceId);

        protected: // members

            boost::shared_ptr<karabo::xms::SignalSlotable> m_signalSlotable;

            bool m_isShared;

            boost::thread m_eventThread;

            FullSchemaCache m_fullSchemaCache;

            CurrentStateSchemaCache m_currentStateSchemaCache;

            ConfigurationCache m_configurationCache;

            DbTableCache m_deviceServerCache;

            DbTableCache m_deviceClassCache;

            DbTableCache m_deviceCache;

            InstanceUsage m_instanceUsage;

            karabo::util::Hash m_deviceChangedHandlers;

            karabo::util::Hash m_propertyChangedHandlers;

            boost::mutex m_schemaCacheMutex;

            boost::mutex m_configurationCacheMutex;

            boost::mutex m_deviceServerCacheMutex;

            boost::mutex m_deviceClassCacheMutex;

            boost::mutex m_deviceCacheMutex;

            boost::mutex m_instanceUsageMutex;

            boost::mutex m_deviceChangedHandlersMutex;

            boost::mutex m_propertyChangedHandlersMutex;

            int m_defaultTimeout;

            std::string m_defaultKeySep;

        };

    }
}


#endif

