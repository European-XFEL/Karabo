/*
 * $Id: Com.hh 6624 2012-06-27 12:57:09Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/xms/SignalSlotable.hh>

#ifndef EXFEL_CORE_DEVCOM_HH
#define	EXFEL_CORE_DEVCOM_HH


namespace exfel {

    namespace core {

        /**
         * The XFEL Device Client
         * This class can be used to (remotely) control devices of the distributed system
         * Synchronous calls (i.e. get()) are in fact asynchronous under the hood
         */
        class DeviceClient {
            
            typedef std::map<std::string, exfel::util::Schema> FullSchemaCache;
            
            typedef std::map<std::string, std::map< std::string, exfel::util::Schema > > CurrentStateSchemaCache;

            typedef std::map<std::string, exfel::util::Hash> ConfigurationCache;
            
            typedef std::vector<exfel::util::Hash> DbTableCache;

            typedef std::map<std::string, unsigned int> InstanceUsage;

            
        public:

            EXFEL_CLASSINFO(DeviceClient, "DeviceClient", "1.0")

                   
            DeviceClient(const std::string& connectionType = "Jms", const exfel::util::Hash& connectionParameters = exfel::util::Hash());

            /**
             * Constructor using instantiated signalSlotable class (shared communication)
             * @param signalSlotable An instance of the SignalSlotable lass
             */
            DeviceClient(const boost::shared_ptr<exfel::xms::SignalSlotable>& signalSlotable);

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
             * Retrieves the full Schema (parameter description) of the given instance;
             * @param instanceId Device instance id
             * @return full Schema
             */
            const exfel::util::Schema& getSchema(const std::string& instanceId, const std::string& key = "", const std::string& keySep = "");
            
            const exfel::util::Schema& getCurrentlyWritableSchema(const std::string& instanceId);
            
            std::vector<std::string> getDeviceServers();
            
            std::vector<std::string> getDeviceClasses(const std::string& deviceServer);
            
            std::vector<std::string> getDevices();
            
            std::vector<std::string> getDeviceParameters(const std::string& instanceId, const std::string& key = "", const std::string& keySep = "");
            
            std::vector<std::string> getDeviceParametersFlat(const std::string& instanceId, const std::string& keySep = "");
            
            std::vector<std::string> getCurrentlyExecutableCommands(const std::string& instanceId, const std::string& keySep = "");
            
            std::vector<std::string> getCurrentlySettableProperties(const std::string& instanceId, const std::string& keySep = "");
            
            bool isCommand(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            bool isProperty(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            bool isChoiceOfNodes(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            bool isListOfNodes(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            bool isNode(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            bool isLeaf(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            bool isAccessInitOnly(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
           
            bool isAccessReconfigurable(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            bool isAccessReadOnly(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            bool isAssignmentOptional(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            bool isAssignmentMandatory(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            std::string getValueTypeAsString(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            std::string getDescription(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            std::string getDisplayedName(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            std::string getDisplayType(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            std::vector<std::string> getAllowedStates(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            std::string getUnitName(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            std::string getUnitSymbol(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            //size_t getMaxSize(const std::string& instanceId, const std::string& key);
            
            std::vector<std::string> getValueOptions(const std::string& instanceId, const std::string& key, const std::string& keySep = "");
            
            void instantiateNoWait(const std::string& serverInstanceId, const std::string& classId, const exfel::util::Hash& configuration);

            std::pair<bool, std::string> instantiateWait(const std::string& serverInstanceId, const std::string& classId, const exfel::util::Hash& configuration = exfel::util::Hash(), int timeout = -1);

            void kill(const std::string& instanceId);

            template<class T>
            T get(const std::string& instanceId, const std::string& key, const std::string& keySep = ".") {
                return cacheAndGetConfiguration(instanceId).getFromPath<T > (key, keySep);
            }

            template<class T>
            void get(const std::string& instanceId, const std::string& key, T& value, const std::string& keySep = ".") {
                return cacheAndGetConfiguration(instanceId).getFromPath(key, value);
            }

            const exfel::util::Hash& get(const std::string& instanceId);

            void get(const std::string& instanceId, exfel::util::Hash& hash);

            template <class ValueType>
            bool registerMonitor(const std::string& instanceId, const std::string& key, const boost::function<void (const ValueType&, const std::string&) >& callbackFunction) {
                exfel::util::Schema schema = this->getSchema(instanceId);
                if (schema.hasKey(key)) {
                    boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                    this->cacheAndGetConfiguration(instanceId);
                    m_propertyChangedHandlers.setFromPath(instanceId + "." + key + "._function", callbackFunction);
                    return true;
                } else {
                    return false;
                }
            }
            
            template <class ValueType, class UserDataType>
            bool registerMonitor(const std::string& instanceId, const std::string& key, const boost::function<void (const ValueType&, const std::string&, const boost::any&) >& callbackFunction, const UserDataType& userData) {
                exfel::util::Schema schema = this->getSchema(instanceId);
                if (schema.hasKey(key)) {
                    boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                    this->cacheAndGetConfiguration(instanceId);
                    m_propertyChangedHandlers.setFromPath(instanceId + "." + key + "._function", callbackFunction);
                    m_propertyChangedHandlers.setFromPath(instanceId + "." + key + "._userData", userData);
                    return true;
                } else {
                    return false;
                }
            }

            void unregisterMonitor(const std::string& instanceId, const std::string& key);

            void registerMonitor(const std::string& instanceId, const boost::function<void (const exfel::util::Hash&, const std::string&)>& callbackFunction);
            
            template <class UserDataType>
            void registerMonitor(const std::string& instanceId, const boost::function<void (const exfel::util::Hash&, const std::string&, const boost::any&)>& callbackFunction, const UserDataType& userData) {
                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                // Make sure we are caching this instanceId
                this->cacheAndGetConfiguration(instanceId);
                m_deviceChangedHandlers.setFromPath(instanceId + "._function", callbackFunction);
                m_deviceChangedHandlers.setFromPath(instanceId + "._userData", userData);
            }

            void unregisterMonitor(const std::string& instanceId);

            template <class T>
            std::pair<bool, std::string> setWait(const std::string& instanceId, const std::string& key, const T& value, const std::string& keySep = ".", int timeout = -1) const {
                exfel::util::Hash tmp;
                tmp.setFromPath(key, value, keySep);
                return setWait(instanceId, tmp, timeout);
            }

            template <class T>
            void setNoWait(const std::string& instanceId, const std::string& key, const T& value, const std::string& keySep = ".") const {
                exfel::util::Hash tmp;
                tmp.setFromPath(key, value, keySep);
                setNoWait(instanceId, tmp);
            }

            std::pair<bool, std::string> setWait(const std::string& instanceId, const exfel::util::Hash& values, int timeout = -1) const;

            void setNoWait(const std::string& instanceId, const exfel::util::Hash& values) const;

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
                } catch (const exfel::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return make_pair(ok, text);
            }

            template <class A1>
            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, int timeout = -1) {
                if (timeout == -1) timeout = m_defaultTimeout;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1).timeout(timeout).receive(text);
                } catch (const exfel::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return make_pair(ok, text);
            }

            template <class A1, class A2>
            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, int timeout = -1) {
                if (timeout == -1) timeout = m_defaultTimeout;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1, a2).timeout(timeout).receive(text);
                } catch (const exfel::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return make_pair(ok, text);
            }

            template <class A1, class A2, class A3>
            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, const A3& a3, int timeout = -1) {
                if (timeout == -1) timeout = m_defaultTimeout;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1, a2, a3).timeout(timeout).receive(text);
                } catch (const exfel::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return make_pair(ok, text);
            }

            template <class A1, class A2, class A3, class A4>
            std::pair<bool, std::string> executeWait(const std::string& instanceId, const std::string& command, const A1& a1, const A2& a2, const A3& a3, const A4& a4, int timeout = -1) {
                if (timeout == -1) timeout = m_defaultTimeout;

                bool ok = true;
                std::string text = "";

                try {
                    m_signalSlotable->request(instanceId, command, a1, a2, a3, a4).timeout(timeout).receive(text);
                } catch (const exfel::util::Exception& e) {
                    text = e.userFriendlyMsg();
                    ok = false;
                }
                return make_pair(ok, text);
            }

        protected: // functions

            static std::string generateOwnInstanceId();
            
            std::vector<std::string> getTopDeviceParameters(const std::string& instanceId);
            
            std::vector<std::string> getSubDeviceParameters(const std::string& instanceId, const std::string& key, const std::string& keySep);
            
            const exfel::util::Schema& getSchemaForParameter(const std::string& instanceId, const std::string& key, const std::string& keySep);
            
            const exfel::util::Schema& getCurrentlyWritableSchemaForParameter(const std::string& instanceId, const std::string& key, const std::string& keySep);
            
            exfel::util::Schema& cacheAndGetFullSchema(const std::string& instanceId);
            
            exfel::util::Schema& cacheAndGetCurrentlyWritableSchema(const std::string& instanceId);

            const exfel::util::Hash& cacheAndGetConfiguration(const std::string& instanceId);
            
            const DbTableCache& cacheAndGetDeviceServers();
            
            const DbTableCache& cacheAndGetDeviceClasses();
            
            const DbTableCache& cacheAndGetDevices();
            
          

            void refreshInstanceUsage(const std::string& instanceId);

            virtual void slotChanged(const exfel::util::Hash& hash, const std::string& instanceId);
            
            virtual void slotNewDeviceServerInstance(const exfel::util::Hash& hash);
            
            virtual void slotUpdateDeviceServerInstance(const exfel::util::Hash& hash);
            
            virtual void slotNewDeviceInstance(const exfel::util::Hash& hash);
            
            virtual void slotUpdateDeviceInstance(const exfel::util::Hash& hash);
            
            virtual void notifyDeviceChangedMonitors(const exfel::util::Hash& hash, const std::string& instanceId);

            virtual void notifyPropertyChangedMonitors(const exfel::util::Hash& hash, const std::string& instanceId);

            void castAndCall(const std::string& instanceId, const exfel::util::Hash& registered, const exfel::util::Hash& current, std::string path = "") const;

            virtual void clearCacheAndDisconnect(const std::string& instanceId);
            
        protected: // members

            boost::shared_ptr<exfel::xms::SignalSlotable> m_signalSlotable;

            bool m_isShared;

            boost::thread m_eventThread;

            FullSchemaCache m_fullSchemaCache;
            
            CurrentStateSchemaCache m_currentStateSchemaCache;

            ConfigurationCache m_configurationCache;
            
            DbTableCache m_deviceServerCache;
            
            DbTableCache m_deviceClassCache;
            
            DbTableCache m_deviceCache;

            InstanceUsage m_instanceUsage;

            exfel::util::Hash m_deviceChangedHandlers;

            exfel::util::Hash m_propertyChangedHandlers;

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

