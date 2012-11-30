/*
 * $Id: Com.cc 6624 2012-06-27 12:57:09Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <map>

#include "DeviceClient.hh"
#include "Device.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::xms;


namespace karabo {
    namespace core {

#define KARABO_REGISTER_CALLBACK(valueType) \
if (itData != entry.end()) {\
    boost::any_cast < boost::function<void (const valueType&, const std::string&, const boost::any&) > >(itFunc->second)(current.get<valueType > (it), instanceId, itData->second);\
} else {\
    boost::any_cast < boost::function<void (const valueType&, const std::string&) > >(itFunc->second)(current.get<valueType > (it), instanceId);\
}

        DeviceClient::DeviceClient(const std::string& connectionType, const karabo::util::Hash& connectionParameters) : m_isShared(false), m_defaultTimeout(8000) {
            karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::create(connectionType, connectionParameters);
            std::string ownInstanceId = generateOwnInstanceId();
            m_signalSlotable = boost::shared_ptr<SignalSlotable > (new SignalSlotable(connection, ownInstanceId));
            m_eventThread = boost::thread(boost::bind(&karabo::xms::SignalSlotable::runEventLoop, m_signalSlotable, true));
            m_signalSlotable->registerSlot<Hash, string > (boost::bind(&karabo::core::DeviceClient::slotChanged, this, _1, _2), "slotChanged");
            m_signalSlotable->registerSlot<Hash > (boost::bind(&karabo::core::DeviceClient::slotNewDeviceServerInstance, this, _1), "slotNewDeviceServerInstance", SignalSlotable::GLOBAL);
            m_signalSlotable->registerSlot<Hash > (boost::bind(&karabo::core::DeviceClient::slotUpdateDeviceServerInstance, this, _1), "slotUpdateDeviceServerInstance", SignalSlotable::GLOBAL);
            m_signalSlotable->registerSlot<Hash > (boost::bind(&karabo::core::DeviceClient::slotNewDeviceInstance, this, _1), "slotNewDeviceInstance", SignalSlotable::GLOBAL);
            m_signalSlotable->registerSlot<Hash > (boost::bind(&karabo::core::DeviceClient::slotUpdateDeviceInstance, this, _1), "slotUpdateDeviceInstance", SignalSlotable::GLOBAL);
        }

        DeviceClient::DeviceClient(const boost::shared_ptr<SignalSlotable>& signalSlotable) : m_signalSlotable(signalSlotable), m_isShared(true), m_defaultTimeout(8000) {
            m_signalSlotable->registerSlot<Hash, string > (boost::bind(&karabo::core::DeviceClient::slotChanged, this, _1, _2), "slotChanged");
            m_signalSlotable->registerSlot<Hash > (boost::bind(&karabo::core::DeviceClient::slotNewDeviceServerInstance, this, _1), "slotNewDeviceServerInstance", SignalSlotable::GLOBAL);
            m_signalSlotable->registerSlot<Hash > (boost::bind(&karabo::core::DeviceClient::slotUpdateDeviceServerInstance, this, _1), "slotUpdateDeviceServerInstance", SignalSlotable::GLOBAL);
            m_signalSlotable->registerSlot<Hash > (boost::bind(&karabo::core::DeviceClient::slotNewDeviceInstance, this, _1), "slotNewDeviceInstance", SignalSlotable::GLOBAL);
            m_signalSlotable->registerSlot<Hash > (boost::bind(&karabo::core::DeviceClient::slotUpdateDeviceInstance, this, _1), "slotUpdateDeviceInstance", SignalSlotable::GLOBAL);
        }

        DeviceClient::~DeviceClient() {
            if (!m_isShared) {
                m_signalSlotable->stopEventLoop();
                m_eventThread.join();
            }
        }

        void DeviceClient::setDefaultTimeout(const unsigned int defaultTimeout) {
            m_defaultTimeout = defaultTimeout;
        }

        int DeviceClient::getDefaultTimeout() const {
            return m_defaultTimeout;
        }

        void DeviceClient::setDefaultKeySeparator(const std::string& defaultKeySep) {
            m_defaultKeySep = defaultKeySep;
        }

        const std::string& DeviceClient::getDefaultKeySeparator() const {
            return m_defaultKeySep;
        }

        std::vector<std::string> DeviceClient::getDeviceServers() {
            vector<string> deviceServers;
            DbTableCache deviceServerCache = cacheAndGetDeviceServers();
            for (size_t i = 0; i < deviceServerCache.size(); ++i) {
                const Hash& deviceServer = deviceServerCache[i];
                if (deviceServer.get<string > ("status") == "online") {
                    deviceServers.push_back(deviceServer.get<string > ("instanceId"));
                }
            }
            return deviceServers;
        }

        const DeviceClient::DbTableCache& DeviceClient::cacheAndGetDeviceServers() {
            boost::mutex::scoped_lock lock(m_deviceServerCacheMutex);

            if (m_deviceServerCache.empty()) {
                m_signalSlotable->request("*", "slotSelect", "id,instanceId,status", "DeviceServerInstance").timeout(m_defaultTimeout).receive(m_deviceServerCache);
            }
            return m_deviceServerCache;
        }

        std::vector<std::string> DeviceClient::getDeviceClasses(const std::string& deviceServerInstanceId) {
            unsigned int devSerInsId;
            bool found = false;
            DbTableCache deviceServerCache = cacheAndGetDeviceServers();
            for (size_t i = 0; i < deviceServerCache.size(); ++i) {
                if (deviceServerCache[i].get<string > ("instanceId") == deviceServerInstanceId) {
                    devSerInsId = deviceServerCache[i].get<unsigned int>("id");
                    found = true;
                    break;
                }
            }
            std::vector<string> deviceClasses;
            if (found) {
                DbTableCache deviceClassCache = cacheAndGetDeviceClasses();
                for (size_t i = 0; i < deviceClassCache.size(); ++i) {
                    if (deviceClassCache[i].get<unsigned int>("devSerInsId") == devSerInsId) {
                        deviceClasses.push_back(deviceClassCache[i].get<string > ("name"));
                    }
                }
            }
            return deviceClasses;
        }

        const DeviceClient::DbTableCache& DeviceClient::cacheAndGetDeviceClasses() {
            boost::mutex::scoped_lock lock(m_deviceClassCacheMutex);

            if (m_deviceClassCache.empty()) {
                m_signalSlotable->request("*", "slotSelect", "devSerInsId,name", "DeviceClass").timeout(m_defaultTimeout).receive(m_deviceClassCache);
            }
            return m_deviceClassCache;
        }

        std::vector<std::string> DeviceClient::getDevices() {
            vector<string> devices;
            DbTableCache deviceCache = cacheAndGetDevices();
            for (size_t i = 0; i < deviceCache.size(); ++i) {
                const Hash& device = m_deviceCache[i];
                devices.push_back(device.get<string > ("instanceId"));
            }
            return devices;
        }

        const DeviceClient::DbTableCache& DeviceClient::cacheAndGetDevices() {
            boost::mutex::scoped_lock lock(m_deviceCacheMutex);

            if (m_deviceCache.empty()) {
                m_signalSlotable->request("*", "slotSelect", "instanceId", "DeviceInstance").timeout(m_defaultTimeout).receive(m_deviceCache);
            }
            return m_deviceCache;
        }

        std::vector<std::string> DeviceClient::getDeviceParameters(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            if (key.empty()) return getTopDeviceParameters(instanceId);
            else return getSubDeviceParameters(instanceId, key, keySep);
        }

        std::vector<std::string> DeviceClient::getTopDeviceParameters(const std::string& instanceId) {
            std::vector<std::string> ret;
            Schema& schema = cacheAndGetFullSchema(instanceId);
            const Schema& params = schema.get<Schema > ("elements");
            for (Schema::const_iterator it = params.begin(); it != params.end(); ++it) {
                const Schema& param = params.get<Schema > (it);
                ret.push_back(param.get<string > ("key"));
            }
            return ret;
        }

        std::vector<std::string> DeviceClient::getSubDeviceParameters(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            std::vector<std::string> ret;
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            if (schema.has("elements")) {
                const Schema& params = schema.get<Schema > ("elements");
                for (Schema::const_iterator it = params.begin(); it != params.end(); ++it) {
                    const Schema& param = params.get<Schema > (it);
                    ret.push_back(param.get<string > ("key"));
                }
            } else if (schema.has("complexType")) {
                const Schema& params = schema.get<Schema > ("complexType");
                for (Schema::const_iterator it = params.begin(); it != params.end(); ++it) {
                    const Schema& param = params.get<Schema > (it);
                    ret.push_back(param.get<string > ("root"));
                }
            }
            return ret;
        }

        std::vector<std::string> DeviceClient::getDeviceParametersFlat(const std::string& instanceId, const std::string& keySep) {
            return cacheAndGetFullSchema(instanceId).getAllParameters();
        }

        const Schema & DeviceClient::getSchemaForParameter(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            return cacheAndGetFullSchema(instanceId).getDescriptionByKey(key);
        }

        std::vector<std::string> DeviceClient::getCurrentlyExecutableCommands(const std::string& instanceId, const std::string& keySep) {
            std::vector<std::string> params = cacheAndGetCurrentlyWritableSchema(instanceId).getAllParameters();
            std::vector<std::string> commands;

            BOOST_FOREACH(std::string param, params) {
                if (this->isCommand(instanceId, param)) {
                    commands.push_back(param);
                }
            }
            return commands;
        }

        std::vector<std::string> DeviceClient::getCurrentlySettableProperties(const std::string& instanceId, const std::string& keySep) {
            std::vector<std::string> params = cacheAndGetCurrentlyWritableSchema(instanceId).getAllParameters();
            std::vector<std::string> properties;

            BOOST_FOREACH(std::string param, params) {
                if (this->isProperty(instanceId, param)) {
                    properties.push_back(param);
                }
            }
            return properties;
        }

        const Schema & DeviceClient::getCurrentlyWritableSchemaForParameter(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            return cacheAndGetCurrentlyWritableSchema(instanceId).getDescriptionByKey(key);
        }

        bool DeviceClient::isCommand(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.isCommand();
        }

        bool DeviceClient::isProperty(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.isAttribute();
        }

        bool DeviceClient::isChoiceOfNodes(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.isChoiceOfNodes();
        }

        bool DeviceClient::isListOfNodes(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.isListOfNodes();
        }

        bool DeviceClient::isNode(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.isNode();
        }

        bool DeviceClient::isLeaf(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.isLeaf();
        }

        bool DeviceClient::isAccessInitOnly(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.isAccessInitOnly();
        }

        bool DeviceClient::isAccessReadOnly(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.isAccessReadOnly();
        }

        bool DeviceClient::isAccessReconfigurable(const std::string& instanceId, const std::string& key, const std::string & keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.isAccessReconfigurable();
        }

        bool DeviceClient::isAssignmentMandatory(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.getAssignment() == Schema::MANDATORY_PARAM;
        }

        bool DeviceClient::isAssignmentOptional(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.getAssignment() == Schema::OPTIONAL_PARAM;
        }

        std::string DeviceClient::getValueTypeAsString(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            return schema.getValueTypeAsString();
        }

        std::string DeviceClient::getDescription(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            if (schema.hasDescription()) return schema.getDescription();
            else return "";
        }

        std::string DeviceClient::getDisplayedName(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            if (schema.hasDisplayedName()) return schema.getDisplayedName();
            else return "";
        }

        std::string DeviceClient::getDisplayType(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            if (schema.hasDisplayType()) return schema.getDisplayType();
            else return "";
        }

        std::vector<std::string> DeviceClient::getAllowedStates(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            if (schema.hasAllowedStates()) return schema.getAllowedStates();
            else return std::vector<std::string > ();
        }

        std::string DeviceClient::getUnitName(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            if (schema.hasUnitName()) return schema.getUnitName();
            else return "";
        }

        std::string DeviceClient::getUnitSymbol(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            if (schema.hasUnitSymbol()) return schema.getUnitSymbol();
            else return "";
        }

        std::vector<std::string> DeviceClient::getValueOptions(const std::string& instanceId, const std::string& key, const std::string& keySep) {
            const Schema& schema = getSchemaForParameter(instanceId, key, keySep);
            if (schema.hasValueOptions()) return schema.getValueOptions();
            else return std::vector<std::string > ();
        }

        Hash DeviceClient::loadConfigurationFromXMLFile(const std::string& filename) {
            Hash configuration, conf;
            conf.setFromPath("TextFile.filename", filename);
            karabo::io::Reader<Hash>::Pointer in = karabo::io::Reader<Hash>::create(conf);
            in->read(configuration);
            return configuration;
        }

        const karabo::util::Schema& DeviceClient::getSchema(const std::string & instanceId, const std::string& key, const std::string& keySep) {
            if (key.empty())
                return cacheAndGetFullSchema(instanceId);
            else
                return getSchemaForParameter(instanceId, key, keySep);
        }

        const karabo::util::Schema& DeviceClient::getCurrentlyWritableSchema(const std::string& instanceId) {
            return cacheAndGetCurrentlyWritableSchema(instanceId);
        }

        void DeviceClient::instantiateNoWait(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash & configuration) {
            Hash tmp(classId, configuration);
            m_signalSlotable->call(serverInstanceId, "slotStartDevice", tmp);
        }
        
        void DeviceClient::instantiateNoWait(const std::string& serverInstanceId, const karabo::util::Hash & completeConfiguration) {
            m_signalSlotable->call(serverInstanceId, "slotStartDevice", completeConfiguration);
        }

        std::pair<bool, std::string > DeviceClient::instantiateWait(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash& configuration, int timeout) {
            if (timeout == -1) timeout = m_defaultTimeout;
            Hash tmp(classId, configuration);
            bool ok = true;
            std::string errorText = "";

            try {
                m_signalSlotable->request(serverInstanceId, "slotStartDevice", tmp).timeout(timeout).receive(ok, errorText);
            } catch (const karabo::util::Exception& e) {
                errorText = e.userFriendlyMsg();
                ok = false;
            }
            return std::make_pair(ok, errorText);
        }

        void DeviceClient::kill(const std::string & instanceId) {
            m_signalSlotable->call(instanceId, "slotKillDeviceInstance");
        }

        const karabo::util::Hash & DeviceClient::get(const std::string & instanceId) {
            return cacheAndGetConfiguration(instanceId);
        }

        void DeviceClient::get(const std::string& instanceId, karabo::util::Hash & hash) {
            hash = cacheAndGetConfiguration(instanceId);
        }

        void DeviceClient::unregisterMonitor(const std::string& instanceId, const std::string & key) {
            boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
            Hash::iterator it = m_propertyChangedHandlers.find(instanceId);
            if (it != m_propertyChangedHandlers.end()) {
                Hash& tmp = m_propertyChangedHandlers.get<Hash > (it);
                Hash::iterator jt = tmp.find(key);
                if (jt != tmp.end()) {
                    tmp.erase(jt);
                }
                if (tmp.empty()) {
                    m_propertyChangedHandlers.erase(it);
                }
            }
        }

        void DeviceClient::registerMonitor(const std::string& instanceId, const boost::function<void (const karabo::util::Hash&, const std::string&)> & callbackFunction) {
            boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
            // Make sure we are caching this instanceId
            this->cacheAndGetConfiguration(instanceId);
            m_deviceChangedHandlers.setFromPath(instanceId + "._function", callbackFunction);
        }

        void DeviceClient::unregisterMonitor(const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
            m_deviceChangedHandlers.erase(instanceId);
        }

        std::pair<bool, std::string > DeviceClient::setWait(const std::string& instanceId, const karabo::util::Hash& values, int timeout) const {
            if (timeout == -1) timeout = m_defaultTimeout;

            bool ok = true;
            std::string errorText = "";

            try {
                // TODO Add error text to response
                m_signalSlotable->request(instanceId, "slotReconfigure", values).timeout(timeout).receive(ok, errorText);
            } catch (const karabo::util::Exception& e) {
                errorText = e.userFriendlyMsg();
                ok = false;
            }
            return std::make_pair(ok, errorText);
        }

        void DeviceClient::setNoWait(const std::string& instanceId, const karabo::util::Hash & values) const {
            m_signalSlotable->call(instanceId, "slotReconfigure", values);
        }

        std::string DeviceClient::generateOwnInstanceId() {
            return std::string(boost::asio::ip::host_name() + "/DeviceClient/" + String::toString(getpid()));
        }

        karabo::util::Schema & DeviceClient::cacheAndGetFullSchema(const std::string & instanceId) {
            FullSchemaCache::iterator it = m_fullSchemaCache.find(instanceId);
            if (it == m_fullSchemaCache.end()) {
                // Request schema
                Schema schema;
                m_signalSlotable->request(instanceId, "slotGetSchema", false).timeout(m_defaultTimeout).receive(schema); // Retrieves full schema
                it = m_fullSchemaCache.insert(make_pair(instanceId, schema)).first;
            }
            return it->second;
        }

        karabo::util::Schema& DeviceClient::cacheAndGetCurrentlyWritableSchema(const std::string& instanceId) {
            std::string state = this->get<std::string > (instanceId, "state");
            CurrentStateSchemaCache::iterator it = m_currentStateSchemaCache.find(instanceId);
            if (it != m_currentStateSchemaCache.end()) {
                CurrentStateSchemaCache::mapped_type& stateCache = it->second;
                CurrentStateSchemaCache::mapped_type::iterator jt = stateCache.find(state);
                if (jt != stateCache.end()) {
                    return jt->second;
                } else {
                    Schema schema;
                    m_signalSlotable->request(instanceId, "slotGetSchema", true).timeout(m_defaultTimeout).receive(schema); // Retrieves current schema
                    jt = stateCache.insert(make_pair(state, schema)).first;
                    return jt->second;
                }
            } else {
                Schema schema;
                m_signalSlotable->request(instanceId, "slotGetSchema", true).timeout(m_defaultTimeout).receive(schema); // Retrieves current schema
                return (m_currentStateSchemaCache[instanceId][state] = schema);
            }
        }

        const karabo::util::Hash & DeviceClient::cacheAndGetConfiguration(const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_configurationCacheMutex);
            ConfigurationCache::iterator it = m_configurationCache.find(instanceId);
            if (it == m_configurationCache.end()) {
                // Request configuration
                Hash hash;
                m_signalSlotable->request(instanceId, "slotRefresh").timeout(m_defaultTimeout).receive(hash);
                it = m_configurationCache.insert(make_pair(instanceId, hash)).first;
                // Keep up to date from now on
                m_signalSlotable->connectR(instanceId, "signalChanged", "", "slotChanged");
            }
            refreshInstanceUsage(instanceId);
            return it->second;
        }

        void DeviceClient::slotNewDeviceServerInstance(const karabo::util::Hash&) {
            boost::mutex::scoped_lock lock(m_deviceServerCacheMutex);
            // TODO This is a simple way here, add however load to the broker and the master
            m_deviceServerCache.clear();
        }

        void DeviceClient::slotUpdateDeviceServerInstance(const karabo::util::Hash&) {
            boost::mutex::scoped_lock lock(m_deviceServerCacheMutex);
            // TODO This is a simple way here, add however load to the broker and the master
            m_deviceServerCache.clear();
        }

        void DeviceClient::slotNewDeviceInstance(const karabo::util::Hash&) {
            boost::mutex::scoped_lock lock(m_deviceCacheMutex);
            m_deviceCache.clear();
        }

        void DeviceClient::slotUpdateDeviceInstance(const karabo::util::Hash&) {
            boost::mutex::scoped_lock lock(m_deviceCacheMutex);
            std::cout << "## Cleared device instance cache ##" << std::endl;
            m_deviceCache.clear();
        }

        void DeviceClient::refreshInstanceUsage(const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_instanceUsageMutex);
            m_instanceUsage[instanceId] = 0;
        }

        void DeviceClient::slotChanged(const karabo::util::Hash& hash, const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_configurationCacheMutex);
            m_configurationCache[instanceId].update(hash);
            // NOTE: This will block us here, i.e. we are deaf for other changes...
            // NOTE: Monitors could be implemented as additional slots, too.
            notifyDeviceChangedMonitors(hash, instanceId);
            notifyPropertyChangedMonitors(hash, instanceId);
        }

        void DeviceClient::notifyDeviceChangedMonitors(const karabo::util::Hash& hash, const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
            Hash::const_iterator it = m_deviceChangedHandlers.find(instanceId);
            if (it != m_deviceChangedHandlers.end()) {
                const Hash& entry = m_deviceChangedHandlers.get<Hash > (it);
                Hash::const_iterator itFunc = entry.find("_function");
                Hash::const_iterator itData = entry.find("_userData");
                if (itData != entry.end()) {
                    boost::any_cast < boost::function<void (const karabo::util::Hash&, const std::string&, const boost::any&)> >(itFunc->second)(hash, instanceId, itData->second);
                } else {
                    boost::any_cast < boost::function<void (const karabo::util::Hash&, const std::string&)> >(itFunc->second)(hash, instanceId);
                }
            }
        }

        void DeviceClient::notifyPropertyChangedMonitors(const karabo::util::Hash& hash, const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
            if (m_propertyChangedHandlers.has(instanceId)) {
                castAndCall(instanceId, m_propertyChangedHandlers.get<karabo::util::Hash > (instanceId), hash);
            }
        }

        void DeviceClient::castAndCall(const std::string& instanceId, const Hash& registered, const Hash& current, std::string path) const {

            for (karabo::util::Hash::const_iterator it = current.begin(); it != current.end(); ++it) {
                std::string currentPath = it->first;
                if (!path.empty()) currentPath = path + "." + it->first;
                if (registered.hasFromPath(currentPath)) {
                    const Hash& entry = registered.getFromPath<Hash > (currentPath);
                    Hash::const_iterator itFunc = entry.find("_function");
                    Hash::const_iterator itData = entry.find("_userData");

                    if (current.is<bool>(it)) {
                        KARABO_REGISTER_CALLBACK(bool);
                    } else if (current.is<char>(it)) {
                        KARABO_REGISTER_CALLBACK(char);
                    } else if (current.is<signed char>(it)) {
                        KARABO_REGISTER_CALLBACK(signed char);
                    } else if (current.is<unsigned char>(it)) {
                        KARABO_REGISTER_CALLBACK(unsigned char);
                    } else if (current.is<short>(it)) {
                        KARABO_REGISTER_CALLBACK(short);
                    } else if (current.is<unsigned short>(it)) {
                        KARABO_REGISTER_CALLBACK(unsigned short);
                    } else if (current.is<int>(it)) {
                        KARABO_REGISTER_CALLBACK(int);
                    } else if (current.is<unsigned int>(it)) {
                        KARABO_REGISTER_CALLBACK(unsigned int);
                    } else if (current.is<unsigned long long>(it)) {
                        KARABO_REGISTER_CALLBACK(unsigned long long);
                    } else if (current.is<float>(it)) {
                        KARABO_REGISTER_CALLBACK(float);
                    } else if (current.is<double>(it)) {
                        KARABO_REGISTER_CALLBACK(double);
                    } else if (current.is<std::string > (it)) {
                        KARABO_REGISTER_CALLBACK(std::string);
                    } else if (current.is<boost::filesystem::path > (it)) {
                        KARABO_REGISTER_CALLBACK(boost::filesystem::path);
                    } else if (current.is<karabo::util::Hash > (it)) {
                        KARABO_REGISTER_CALLBACK(karabo::util::Hash);
                    } else if (current.is < std::deque<bool> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::deque<bool>);
                    } else if (current.is<std::vector<char> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<char>);
                    } else if (current.is < std::vector<signed char> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<signed char>);
                    } else if (current.is<std::vector<unsigned char> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned char>);
                    } else if (current.is<std::vector<short> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<short>);
                    } else if (current.is<std::vector<unsigned short> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned short>);
                    } else if (current.is<std::vector<int> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<int>);
                    } else if (current.is<std::vector<unsigned int> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned int>);
                    } else if (current.is<std::vector<long long> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<long long>);
                    } else if (current.is<std::vector<unsigned long long> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned long long>);
                    } else if (current.is<std::vector<float> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<float>);
                    } else if (current.is<std::vector<double> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<double>);
                    } else if (current.is<karabo::util::Schema > (it)) {
                        KARABO_REGISTER_CALLBACK(karabo::util::Schema);
                    } else if (current.is<std::vector<std::string> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<std::string>);
                    } else if (current.is<std::vector<karabo::util::Hash> >(it)) {
                        KARABO_REGISTER_CALLBACK(std::vector<karabo::util::Hash>);

                    } else {
                        throw LOGIC_EXCEPTION("Failed to call registered monitored (datatype problems)");
                    }
                }
                if (current.is<karabo::util::Hash > (it)) castAndCall(instanceId, registered, current.get<Hash > (it), currentPath);
            }
        }

        void DeviceClient::clearCacheAndDisconnect(const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_configurationCacheMutex);
            m_signalSlotable->disconnect(instanceId, "signalChanged", "", "slotChanged", false);
            m_configurationCache.erase(instanceId);
        }


    }
}
