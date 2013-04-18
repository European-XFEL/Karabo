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

            typedef std::map<std::string, unsigned int> InstanceUsage;

        protected: // members

            /**
             * server +
             *   <serverId> type host deviceClasses version +
             *     classes +
             *       <classId> +
             *         description SCHEMA
             *         configuration HASH
             *     configuration HASH
             * device +
             *   <deviceId> type host classId serverId version +
             *      fullSchema SCHEMA
             *      activeSchema +
             *         <stateName> +
             *           <roleName> SCHEMA
             *      configuration HASH
             */
            karabo::util::Hash m_runtimeSystemDescription;

            boost::mutex m_runtimeSystemDescriptionMutex;

            std::string m_role;
            
            boost::shared_ptr<karabo::xms::SignalSlotable> m_signalSlotable;

            bool m_isShared;

            boost::thread m_eventThread;

            InstanceUsage m_instanceUsage;

            karabo::util::Hash m_deviceChangedHandlers;

            karabo::util::Hash m_propertyChangedHandlers;

            boost::mutex m_instanceUsageMutex;

            boost::mutex m_deviceChangedHandlersMutex;

            boost::mutex m_propertyChangedHandlersMutex;

            int m_defaultTimeout;

        public:

            KARABO_CLASSINFO(DeviceClient, "DeviceClient", "1.0");

            /**
             * Constructor which establishes an own connection to the communication system (default JMS - OpenMQ)
             * @param connectionType The communication system transport layer implementation
             * @param connectionParameters Additional connection configuration
             */
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

           
            /**
             * Allows asking whether an instance is online in the current distributed system
             * @param boolean indicating whether existing and hostname if exists
             * @return 
             */
            std::pair<bool, std::string> exists(const std::string& instanceId);

            /**
             * Retrieves all device servers currently existing in the distributed system
             * @return array of device server ids
             */
            std::vector<std::string> getDeviceServers();

            /**
             * Retrieves all device classes (plugins) available on a given device server
             * @param deviceServer device server id
             * @return array of device classes
             */
            std::vector<std::string> getDeviceClasses(const std::string& deviceServer);

            std::vector<std::string> getDevices(const std::string& deviceServer);

            std::vector<std::string> getDevices();

            /**
             * Retrieves the full Schema (parameter description) of the given instance;
             * @param instanceId Device's instance ID
             * @return full Schema
             */
            karabo::util::Schema getFullSchema(const std::string& instanceId);

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
             * @param deviceServer instanceId of a deviceServer
             * @param className name of loaded class on the deviceServer (classId)
             * @return Schema describing parameters available at instantiation time
             */
            karabo::util::Schema getClassSchema(const std::string& deviceServer, const std::string& className);

            std::vector<std::string> getCurrentlyExecutableCommands(const std::string& instanceId);

            std::vector<std::string> getCurrentlySettableProperties(const std::string& instanceId);

            //karabo::util::Hash loadConfigurationFromDB(const std::string& configurationId);

            karabo::util::Hash loadConfigurationFromFile(const std::string& filename);

            void instantiateNoWait(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash& configuration = karabo::util::Hash());

            void instantiateNoWait(const std::string& serverInstanceId, const karabo::util::Hash& configuration);

            //            std::pair<bool, std::string> instantiateWait(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash& configuration = karabo::util::Hash(), int timeout = -1);
            //
            //            std::pair<bool, std::string> instantiateWait(const std::string& serverInstanceId, const karabo::util::Hash& configuration, int timeout = -1);

            void killNoWait(const std::string& instanceId);

            std::pair<bool, std::string> killWait(const std::string& instanceId);

            karabo::util::Hash get(const std::string& instanceId);

            void get(const std::string& instanceId, karabo::util::Hash& hash);

            template<class T>
            T get(const std::string& instanceId, const std::string& key, const char keySep = '.') {
                return cacheAndGetConfiguration(instanceId).get<T > (key, keySep);
            }

            template<class T>
            void get(const std::string& instanceId, const std::string& key, T& value, const char keySep = '.') {
                return cacheAndGetConfiguration(instanceId).get(key, value, keySep);
            }

            template <class ValueType>
            bool registerPropertyMonitor(const std::string& instanceId, const std::string& key, const boost::function<void (const ValueType&, const std::string&) >& callbackFunction) {
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
            bool registerPropertyMonitor(const std::string& instanceId, const std::string& key, const boost::function<void (const ValueType&, const std::string&, const boost::any&) >& callbackFunction, const UserDataType& userData) {
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

            void unregisterPropertyMonitor(const std::string& instanceId, const std::string& key);

            void registerDeviceMonitor(const std::string& instanceId, const boost::function<void (const karabo::util::Hash&, const std::string&)>& callbackFunction);

            template <class UserDataType>
            void registerDeviceMonitor(const std::string& instanceId, const boost::function<void (const karabo::util::Hash&, const std::string&, const boost::any&)>& callbackFunction, const UserDataType& userData) {
                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                // Make sure we are caching this instanceId
                this->cacheAndGetConfiguration(instanceId);
                m_deviceChangedHandlers.set(instanceId + "._function", callbackFunction);
                m_deviceChangedHandlers.set(instanceId + "._userData", userData);
            }

            void unregisterDeviceMonitor(const std::string& instanceId);

            template <class T>
            std::pair<bool, std::string> setWait(const std::string& instanceId, const std::string& key, const T& value, int timeout = -1, const char keySep = '.') const {
                karabo::util::Hash tmp;
                tmp.set(key, value, keySep);
                return setWait(instanceId, tmp, timeout);
            }

            template <class T>
            void setNoWait(const std::string& instanceId, const std::string& key, const T& value, const char keySep = '.') const {
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
            //
            //            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, int timeout = -1) {
            //                if (timeout == -1) timeout = m_defaultTimeout;
            //
            //                bool ok = true;
            //                std::string text = "";
            //
            //                try {
            //                    m_signalSlotable->request(instanceId, command).timeout(timeout).receive(text);
            //                } catch (const karabo::util::Exception& e) {
            //                    text = e.userFriendlyMsg();
            //                    ok = false;
            //                }
            //                return std::make_pair(ok, text);
            //            }
            //
            //            template <class A1>
            //            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, int timeout = -1) {
            //                if (timeout == -1) timeout = m_defaultTimeout;
            //
            //                bool ok = true;
            //                std::string text = "";
            //
            //                try {
            //                    m_signalSlotable->request(instanceId, command, a1).timeout(timeout).receive(text);
            //                } catch (const karabo::util::Exception& e) {
            //                    text = e.userFriendlyMsg();
            //                    ok = false;
            //                }
            //                return std::make_pair(ok, text);
            //            }
            //
            //            template <class A1, class A2>
            //            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, int timeout = -1) {
            //                if (timeout == -1) timeout = m_defaultTimeout;
            //
            //                bool ok = true;
            //                std::string text = "";
            //
            //                try {
            //                    m_signalSlotable->request(instanceId, command, a1, a2).timeout(timeout).receive(text);
            //                } catch (const karabo::util::Exception& e) {
            //                    text = e.userFriendlyMsg();
            //                    ok = false;
            //                }
            //                return std::make_pair(ok, text);
            //            }
            //
            //            template <class A1, class A2, class A3>
            //            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, const A3& a3, int timeout = -1) {
            //                if (timeout == -1) timeout = m_defaultTimeout;
            //
            //                bool ok = true;
            //                std::string text = "";
            //
            //                try {
            //                    m_signalSlotable->request(instanceId, command, a1, a2, a3).timeout(timeout).receive(text);
            //                } catch (const karabo::util::Exception& e) {
            //                    text = e.userFriendlyMsg();
            //                    ok = false;
            //                }
            //                return std::make_pair(ok, text);
            //            }
            //
            //            template <class A1, class A2, class A3, class A4>
            //            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, const A3& a3, const A4& a4, int timeout = -1) {
            //                if (timeout == -1) timeout = m_defaultTimeout;
            //
            //                bool ok = true;
            //                std::string text = "";
            //
            //                try {
            //                    m_signalSlotable->request(instanceId, command, a1, a2, a3, a4).timeout(timeout).receive(text);
            //                } catch (const karabo::util::Exception& e) {
            //                    text = e.userFriendlyMsg();
            //                    ok = false;
            //                }
            //                return std::make_pair(ok, text);
            //            }

        protected: // functions

            void cacheAvailableInstances();
            
            virtual void setupSlots();
            
            virtual void slotChanged(const karabo::util::Hash& hash, const std::string& instanceId);
            
            virtual void slotInstanceUpdated(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
             
            virtual void slotInstanceGone(const std::string& instanceId);

            static std::string generateOwnInstanceId();

            karabo::util::Schema cacheAndGetFullSchema(const std::string& instanceId);

            karabo::util::Schema cacheAndGetActiveSchema(const std::string& instanceId);

            karabo::util::Hash cacheAndGetConfiguration(const std::string& instanceId);
            
            void refreshInstanceUsage(const std::string& instanceId);

            virtual void notifyDeviceChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId);

            virtual void notifyPropertyChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId);

            void castAndCall(const std::string& instanceId, const karabo::util::Hash& registered, const karabo::util::Hash& current, std::string path = "") const;

            virtual void clearCacheAndDisconnect(const std::string& instanceId);



        };

    }
}


#endif

