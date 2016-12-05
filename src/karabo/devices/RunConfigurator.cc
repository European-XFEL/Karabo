#include <string>
#include "RunConfigurator.hh"
#include "RunConfigurationGroup.hh"
#include "AlarmService.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::core;
using namespace karabo::net;
using namespace karabo::io;
using namespace karabo::xms;


namespace karabo {
    namespace devices {


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, RunConfigurator)


        void RunConfigurator::expectedParameters(Schema& expected) {

            SLOT_ELEMENT(expected).key("buildConfigurationInUse")
                    .displayedName("Push to DAQ")
                    .description("Push current configuration structure to the DAQ Run controller.")
                    .allowedStates(State::NORMAL)
                    .commit();

            Schema availRow;

            STRING_ELEMENT(availRow).key("groupId")
                    .displayedName("Group")
                    .description("Run configuration group name.")
                    .assignmentMandatory()
                    .reconfigurable()
                    .commit();

            STRING_ELEMENT(availRow).key("description")
                    .displayedName("Description")
                    .description("Run configuration group description.")
                    .assignmentOptional().defaultValue("")
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(availRow).key("use")
                    .displayedName("Use")
                    .description("Run configuration group usage flag.")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();

            TABLE_ELEMENT(expected).key("availableGroups")
                    .displayedName("Available group configurations")
                    .setColumns(availRow)
                    .assignmentOptional().noDefaultValue()
                    .reconfigurable()
                    .commit();

            STRING_ELEMENT(expected).key("currentGroup")
                    .displayedName("Sources in group")
                    .description("Current group identifier")
                    .options("")
                    .assignmentOptional().defaultValue("")
                    .reconfigurable()
                    .commit();

            Schema sourceOnlyRow;

            STRING_ELEMENT(sourceOnlyRow).key("sourceOnly")
                    .displayedName("Source")
                    .description("Data source full name.")
                    .readOnly()
                    .commit();

            TABLE_ELEMENT(expected).key("sourcesOnly")
                    .displayedName("")
                    .description("List of data sources")
                    .setColumns(sourceOnlyRow)
                    .assignmentOptional().noDefaultValue()
                    .reconfigurable()
                    .commit();

            Schema sourceRow;

            BOOL_ELEMENT(sourceRow).key("use")
                    .displayedName("Use")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();

            TABLE_ELEMENT(expected).key("sources")
                    .displayedName("Compiled source List")
                    .description("Overall list of data source and their attributes")
                    .addColumnsFromClass<RunControlDataSource>()
                    .addColumns(sourceRow)
                    .assignmentOptional().noDefaultValue()
                    .reconfigurable()
                    .commit();

        }


        RunConfigurator::RunConfigurator(const karabo::util::Hash& input)
            : Device(input)
            , m_configurations()
            , m_cursor()
            , m_deviceIds()
            , m_groupIds() { // TODO:  This is for DEBUGGING purposes ONLY!  Make it empty!

            KARABO_INITIAL_FUNCTION(initialize);

        }


        RunConfigurator::~RunConfigurator() {
            m_configurations.clear();
        }


        void RunConfigurator::initialize() {
            updateState(State::INIT);

            KARABO_SIGNAL2("signalRunConfiguration", karabo::util::Hash /*configuration*/, string /*deviceId*/)
            KARABO_SLOT(buildConfigurationInUse);
            KARABO_SLOT(updateAvailableGroups);
            KARABO_SLOT(updateCurrentGroup);

            m_configurations.clear();

            // Switch on the heartbeat tracking 
            trackAllInstances();
            // First call : trigger the process of gathering the info about network presence
            remote().getSystemInformation();

            // Register handlers here: it will switch on multi-threading!

            remote().registerInstanceNewMonitor(boost::bind(&RunConfigurator::newDeviceHandler, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&RunConfigurator::deviceGoneHandler, this, _1, _2));

            initAvailableGroups();

            updateCurrentGroupSchema();

            printConfig();

            updateState(State::NORMAL);
        }
        
        void RunConfigurator::updateCurrentGroupSchema() {
            m_deviceIds = getConfigurationGroupDeviceIds();
            m_groupIds = getConfigurationGroupIds();
            const std::string currentGroup = get<std::string>("currentGroup");
            Schema schema = getFullSchema();
            schema.setOptions("currentGroup", toString(m_groupIds), ",");
            this->updateSchema(schema, true);
            if(std::find(m_groupIds.begin(), m_groupIds.end(), currentGroup) != m_groupIds.end()){
                set("currentGroup", currentGroup);
            }

        }


        void RunConfigurator::printConfig() const {

            KARABO_LOG_FRAMEWORK_DEBUG << "\n\nConfigurations are ...\n";
            for (MapGroup::const_iterator it = m_configurations.begin(); it != m_configurations.end(); ++it) {
                const string& deviceId = it->first;
                const Hash& group = it->second;
                KARABO_LOG_FRAMEWORK_DEBUG << "deviceId:" << deviceId << ", groupId:" << group.get<string>("id")
                        << ", desc:" << (group.has("group") ? group.get<string>("description") : "" )<< ", use:" << group.get<bool>("use");
                KARABO_LOG_FRAMEWORK_DEBUG << "\tExpert";
                const vector<Hash>& expert = group.get<vector < Hash >> ("expert");
                for (vector<Hash>::const_iterator ii = expert.begin(); ii != expert.end(); ++ii) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "\tsource:" << ii->get<string>("source")
                            << ", type:" << ii->get<string>("type")
                            << ", behavior:" << ii->get<string>("behavior")
                            << ", monitored:" << ii->get<bool>("monitored")
                            << ", use:" << ii->get<bool>("use");
                }
                KARABO_LOG_FRAMEWORK_DEBUG << "\tUser";
                const vector<Hash>& user = group.get<vector < Hash >> ("user");
                for (vector<Hash>::const_iterator ii = user.begin(); ii != user.end(); ++ii) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "\tsource:" << ii->get<string>("source")
                            << ", type:" << ii->get<string>("type")
                            << ", behavior:" << ii->get<string>("behavior")
                            << ", monitored:" << ii->get<bool>("monitored")
                            << ", use:" << ii->get<bool>("use");
                }
            }
        }


        std::vector<std::string> RunConfigurator::getConfigurationGroupDeviceIds() const {
            vector<string> ids;
            for (MapGroup::const_iterator it = m_configurations.begin(); it != m_configurations.end(); ++it) {
                ids.push_back(it->first);
            }
            return ids;
        }


        std::vector<std::string> RunConfigurator::getConfigurationGroupIds() const {
            vector<string> ids;
            for (MapGroup::const_iterator it = m_configurations.begin(); it != m_configurations.end(); ++it) {
                const Hash& hash = it->second;
                if (hash.has("id")) ids.push_back(hash.get<string>("id"));
            }
            return ids;
        }


        const std::string& RunConfigurator::getDeviceIdByGroupId(const std::string& groupId, const std::string& failure) const {
            assert(m_groupIds.size() == m_deviceIds.size());
            for (size_t i = 0; i < m_groupIds.size(); i++) {
                if (m_groupIds[i] == groupId)
                    return m_deviceIds[i];
            }
            return failure;
        }


        void RunConfigurator::initAvailableGroups() {

            const Hash runtimeInfo = remote().getSystemInformation();

            KARABO_LOG_FRAMEWORK_DEBUG << "\ninitAvailableGroups:   runtimeInfo ....\n" << runtimeInfo;

            if (!runtimeInfo.has("device")) return;

            const Hash& onlineDevices = runtimeInfo.get<Hash>("device");
            for (Hash::const_iterator i = onlineDevices.begin(); i != onlineDevices.end(); ++i) {
                const Hash::Node& deviceNode = *i;
                // Topology entry as understood by newDeviceHandler: Hash with path "device.<deviceId>"
                Hash topologyEntry("device", Hash());
                // Copy node with key "<deviceId>" and attributes into the single Hash in topologyEntry:
                topologyEntry.begin()->getValue<Hash>().setNode(deviceNode);
                newDeviceHandler(topologyEntry);
            }
        }


        void RunConfigurator::newDeviceHandler(const karabo::util::Hash & topologyEntry) {
            try {
                const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
                // const ref is fine even for temporary std::string
                const std::string& instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                                                 topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));
                if (type != "device") return;

                const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                const string& deviceId = instanceId;

                if (!entry.hasAttribute(deviceId, "classId") || entry.getAttribute<string>(deviceId, "classId") != "RunConfigurationGroup")
                    return;

                // Add new configuration group into the map
                m_configurations[deviceId] = Hash();
                if (m_cursor.empty()) m_cursor = deviceId;
                Hash& group = m_configurations[deviceId];
                remote().get(deviceId, "group", group);
                group.set("use", false);
                if (!group.has("expert"))
                    group.set("expert", vector<Hash>());
                else {
                    vector<Hash>& v = group.get<vector < Hash >> ("expert");
                    for (size_t i = 0; i < v.size(); i++) v[i].set("use", false);
                }
                if (!group.has("user"))
                    group.set("user", vector<Hash>());
                else {
                    vector<Hash>& v = group.get<vector < Hash >> ("user");
                    for (size_t i = 0; i < v.size(); i++) v[i].set("use", false);
                }

                KARABO_LOG_FRAMEWORK_DEBUG << "New RunConfigurationGroup --> instanceId: '" << instanceId
                        << "',  cursor is '" << m_cursor << "' ...\n" << group;

                updateAvailableGroups();

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "In newDeviceHandler:\n" << e;
            } catch (const std::exception& e) {
                KARABO_LOG_ERROR << "In newDeviceHandler: " << e.what() << ".";
            } catch (...) {
                KARABO_LOG_ERROR << "Unknown exception in newDeviceHandler.";
            }
        }


        void RunConfigurator::deviceGoneHandler(const std::string& instanceId, const karabo::util::Hash & instanceInfo) {
            const std::string& type = (instanceInfo.has("type") && instanceInfo.is<std::string>("type") ?
                                       instanceInfo.get<std::string>("type") : std::string("unknown"));
            const std::string& classId = (instanceInfo.has("classId") && instanceInfo.is<std::string>("classId") ?
                                          instanceInfo.get<string>("classId") : std::string("?"));

            if (type != "device" || classId != "RunConfigurationGroup") return;

            KARABO_LOG_FRAMEWORK_DEBUG << "instanceGoneHandler -->  instanceId  '" << instanceId << "' is erased.";

            m_configurations.erase(instanceId);

            updateAvailableGroups();
        }


        void RunConfigurator::updateAvailableGroups() {
            typedef map<string, Hash> MapGroup;

            vector<Hash> g;

            for (MapGroup::iterator i = m_configurations.begin(); i != m_configurations.end(); ++i) {
                Hash h;
                const string& deviceId = i->first;

                h.set("groupId", i->second.get<string>("id"));
                if (i->second.has("description"))
                    h.set("description", i->second.get<string>("description"));
                else
                    h.set<string>("description", "");
                if (i->second.has("use"))
                    h.set("use", i->second.get<bool>("use"));
                else
                    h.set("use", false);

                g.push_back(h);

                if (deviceId == m_cursor) {
                    updateCurrentGroup();
                }
            }
            
            updateCurrentGroupSchema();
            set("availableGroups", g);
        }


        void RunConfigurator::updateCurrentGroup() {
            setNoValidate("currentGroup", m_configurations[m_cursor].get<string>("id"));
            updateCurrentSources();
            updateCompiledSourceList();
            
        }


        void RunConfigurator::updateCurrentSources() {
            const Hash& g = m_configurations[m_cursor];

            KARABO_LOG_FRAMEWORK_DEBUG << "updateCurrentSources()";
            vector<Hash> sources;

            const vector<Hash>& expert = g.get<vector<Hash> >("expert");
            for (size_t i = 0; i < expert.size(); i++) {
                const string& src = expert[i].get<string>("source");
                sources.push_back(Hash("sourceOnly", src));
            }

            const vector<Hash>& user = g.get<vector<Hash> >("user");
            for (size_t ii = 0; ii < user.size(); ii++) {
                const string& src = user[ii].get<string>("source");
                sources.push_back(Hash("sourceOnly", src));
            }

            set("sourcesOnly", sources);
        }


        void RunConfigurator::updateCompiledSourceList() {
            std::map<std::string, Hash> sources;
            for(auto it = m_configurations.cbegin(); it != m_configurations.cend(); ++it) {
                const Hash& g = it->second;
                bool use = g.get<bool>("use");

                KARABO_LOG_FRAMEWORK_DEBUG << "updateCompiledSourceList()  cursor : " << it->first << ", use : " << use;

                if(use){
                    
                    const vector<Hash>& expert = g.get<vector<Hash> >("expert");
                    for (size_t i = 0; i < expert.size(); i++) {
                        const string& src = expert[i].get<string>("source");
                        const bool pipeline = (expert[i].hasAttribute("source", "pipeline") ? expert[i].getAttribute<bool>("source", "pipeline"): false);
                        const string& type = expert[i].get<string>("type");
                        const string& behavior = expert[i].get<string>("behavior");
                        const bool monitored = expert[i].get<bool>("monitored");
                        Hash h("source", src
                               , "type", type
                               , "behavior", behavior
                               , "monitored", monitored
                               , "use", use);
                        h.setAttribute("source", "pipeline", pipeline);
                        auto exIt = sources.find(src);
                        if(exIt != sources.end()){
                            const std::string& exbehavior = exIt->second.get<std::string>("behavior");
                            bool exmonitored = exIt->second.get<bool>("monitored");
                            if (exmonitored) h.set("monitored", true);
                            if (behavior == "init" || (behavior == "read-only" && exbehavior != "init")) {
                                h.set("behavior", exbehavior);
                            }
                        }
                        sources[src] = std::move(h);
                    }

                    const vector<Hash>& user = g.get<vector<Hash> >("user");
                    for (size_t ii = 0; ii < user.size(); ii++) {
                        const string& src = user[ii].get<string>("source");
                        const bool pipeline = (user[ii].hasAttribute("source", "pipeline") ? user[ii].getAttribute<bool>("source", "pipeline"): false);
                        const string& type = user[ii].get<string>("type");
                        const string& behavior = user[ii].get<string>("behavior");
                        const bool monitored = user[ii].get<bool>("monitored");
                        Hash h("source", src
                               , "type", type
                               , "behavior", behavior
                               , "monitored", monitored
                               , "use", use);
                        h.setAttribute("source", "pipeline", pipeline);
                        auto exIt = sources.find(src);
                        if(exIt != sources.end()){
                            const std::string& exbehavior = exIt->second.get<std::string>("behavior");
                            bool exmonitored = exIt->second.get<bool>("monitored");
                            if (exmonitored) h.set("monitored", true);
                            if (behavior == "init" || (behavior == "read-only" && exbehavior != "init")) {
                                h.set("behavior", exbehavior);
                            }
                        }
                        sources[src] = std::move(h);
                    }
                }
            }
            std::vector<Hash> sourceVec;
            for(auto it = sources.cbegin(); it != sources.cend(); ++it){
                sourceVec.push_back(it->second);
            }
            set("sources", sourceVec);
        }


        std::vector<karabo::util::Hash> RunConfigurator::getAllSources(const std::string & deviceId) const {

            KARABO_LOG_FRAMEWORK_DEBUG << "getAllSources{\"" << deviceId << "\")";

            std::vector<karabo::util::Hash> v;
            MapGroup::const_iterator it = m_configurations.find(deviceId);

            if (it != m_configurations.end()) {
                const Hash& g = it->second;

                const std::vector<karabo::util::Hash>& expert = g.get<std::vector < karabo::util::Hash >> ("expert");
                std::copy(expert.begin(), expert.end(), v.begin());

                const std::vector<karabo::util::Hash>& user = g.get<std::vector < karabo::util::Hash >> ("user");
                v.insert(v.end(), user.begin(), user.end());
            }

            return v;
        }


        std::vector<std::string> RunConfigurator::getAllSourceNames(const std::string & deviceId) const {

            KARABO_LOG_FRAMEWORK_DEBUG << "getAllSourceNames(\"" << deviceId << "\")";

            std::vector<std::string> v;
            MapGroup::const_iterator it = m_configurations.find(deviceId);

            if (it != m_configurations.end()) {
                for (const auto& h : it->second.get<std::vector < karabo::util::Hash >> ("expert")) {
                    if (h.has("source")) v.push_back(h.get<std::string>("source"));
                }
                for (const auto& h : it->second.get<std::vector < karabo::util::Hash >> ("user")) {
                    if (h.has("source")) v.push_back(h.get<std::string>("source"));
                }
            }

            return v;
        }


        void RunConfigurator::buildConfigurationInUse() {

            KARABO_LOG_FRAMEWORK_DEBUG << "buildConfigurationInUse()";

            Hash configuration("configuration", Hash());
            Hash& result = configuration.get<Hash>("configuration");

            for (MapGroup::const_iterator it = m_configurations.begin(); it != m_configurations.end(); ++it) {
                const string& deviceId = it->first;
                const Hash& group = it->second;
                const bool useGroupFlag = group.get<bool>("use");
                if (useGroupFlag) {
                    const string& id = group.get<string>("id");
                    buildDataSourceProperties(group.get<vector < Hash >> ("expert"), id, true, false, result);
                    buildDataSourceProperties(group.get<vector < Hash >> ("user"),   id, false, true, result);
                }
            }

            KARABO_LOG_FRAMEWORK_INFO << "Current Run Configuration is ...\n" << configuration;

            emit("signalRunConfiguration", configuration, getInstanceId());
            
            karabo::io::saveToFile<Hash>(configuration, "lastRunConfiguration.xml", Hash("format.Xml.indentation", 2, "format.Xml.writeDataTypes", true));
        }


        void RunConfigurator::buildDataSourceProperties(const std::vector<karabo::util::Hash>& table,
                                                        const std::string& groupId, bool expertFlag, bool userFlag,
                                                        karabo::util::Hash& result) {

            for (vector<Hash>::const_iterator ii = table.begin(); ii != table.end(); ++ii) {
                const string& dataSourceId = ii->get<string>("source");
                const bool pipelineFlag = ii->getAttribute<bool>("source", "pipeline");
                const string& dataSourceType = ii->get<string>("type");
                string behavior = ii->get<string>("behavior");
                const bool monitorOut = ii->get<bool>("monitored");
                const bool inUse = ii->get<bool>("use");

                KARABO_LOG_FRAMEWORK_DEBUG << "buildDataSourceProperties dataSourceId : " << dataSourceId << ", pipeline : " << pipelineFlag;

                // Inclusiveness for "behavior" attribute
                // "smart" merging where "high-grade" behavior attribute overwrites "low-grade" one.
                if (result.has(dataSourceId)) {
                    const string& oldBehavior = result.getAttribute<string>(dataSourceId, "behavior");
                    if (behavior == "init" || (behavior == "read-only" && oldBehavior == "record-all"))
                        behavior = oldBehavior;
                }

                if (inUse) {
                    Hash properties;
                    // It was decided not to send all properties to the PCLayer.
                    // The call to 'getDataSourceSchemaAsHash()' will be done by PCLayer software like ...
                    //----------------------------------------------------------------------
                    //int access = 0;
                    //if (behavior == "record-all") access = INIT|READ|WRITE;
                    //else if (behavior == "read-only") access = INIT|READ;
                    //else access = INIT;
                    //remote().getDataSourceSchemaAsHash(dataSourceId, properties, access);
                    //----------------------------------------------------------------------
                    // The PCLayer software may call this many times ...

                    // Instead here we just send a stub ("data source" granularity level)
                    properties.set(dataSourceId, Hash());

                    properties.setAttribute(dataSourceId, "configurationGroupId", groupId);
                    properties.setAttribute(dataSourceId, "pipeline", pipelineFlag);
                    properties.setAttribute(dataSourceId, "expertData", expertFlag);
                    properties.setAttribute(dataSourceId, "userData", userFlag);
                    properties.setAttribute(dataSourceId, "behavior", behavior);
                    properties.setAttribute(dataSourceId, "monitorOut", monitorOut);
                    result.merge(properties, Hash::REPLACE_ATTRIBUTES);
                }
            }
        }


        void RunConfigurator::preReconfigure(karabo::util::Hash & incomingReconfiguration) {
            KARABO_LOG_FRAMEWORK_DEBUG << "============ preReconfigure  ===============";
            const Schema schema = getFullSchema();
            if (incomingReconfiguration.has("currentGroup")) {
                const std::string& currentGroup = incomingReconfiguration.get<string>("currentGroup");
                string oldCursor = m_cursor;
                KARABO_LOG_FRAMEWORK_DEBUG << "============ preReconfigure cursor WAS ==> " << m_cursor;
                m_cursor = getDeviceIdByGroupId(currentGroup);
                KARABO_LOG_FRAMEWORK_DEBUG << "============ preReconfigure cursor NOW ==> " << m_cursor;
                if (oldCursor != m_cursor) {
                    // Clear old stuff
                    set("sourceOnly", vector<Hash>());
                    set("sources", vector<Hash>());
                }
                updateCurrentSources();
                updateCompiledSourceList();
            }
            for (Hash::const_iterator it = incomingReconfiguration.begin(); it != incomingReconfiguration.end(); ++it) {
                const string& wid = it->getKey();
                if (it->getType() == Types::VECTOR_HASH) {
                    if (schema.hasDisplayType(wid) && schema.getDisplayType(wid) == "Table") {
                        const vector<Hash>& value = it->getValue<vector < Hash >> ();
                        Schema rowSchema = schema.getParameterHash().getAttribute<Schema>(wid, KARABO_SCHEMA_ROW_SCHEMA);
                        if (wid == "availableGroups") {
                            reconfigureAvailableGroups(value, rowSchema);
                            updateCompiledSourceList();
                        } else if (wid == "sourcesOnly")
                            reconfigureSourcesOnly(value, rowSchema);
                        else if (wid == "sources")
                            reconfigureSources(value, rowSchema);
                    }
                }
            }
            KARABO_LOG_FRAMEWORK_DEBUG << "============  preReconfigure end ============\n";
        }


        void RunConfigurator::postReconfigure() {
            KARABO_LOG_FRAMEWORK_DEBUG << "************ postReconfigure ***************";
            KARABO_LOG_FRAMEWORK_DEBUG << "************ availableGroups ***************\n";
            const vector<Hash>& groups = get<vector < Hash >> ("availableGroups");
            for (const auto& h : groups) KARABO_LOG_FRAMEWORK_DEBUG << "...\n" << h;

            KARABO_LOG_FRAMEWORK_DEBUG << "************ sources         ***************\n";
            const vector<Hash>& sources = get<vector < Hash >> ("sources");
            for (const auto& h : sources) KARABO_LOG_FRAMEWORK_DEBUG << "...\n" << h;

            printConfig();

            KARABO_LOG_FRAMEWORK_DEBUG << "********************************************\n\n\n";
        }


        void RunConfigurator::reconfigureAvailableGroups(const std::vector<karabo::util::Hash>& groups, const karabo::util::Schema & schema) {
            for (vector<Hash>::const_iterator it = groups.begin(); it != groups.end(); ++it) {
                const Hash& hash = *it;
                const string& groupId = hash.get<string>("groupId");
                const string& deviceId = getDeviceIdByGroupId(groupId);
                const bool useFlag = hash.get<bool>("use");
                MapGroup::iterator ii = m_configurations.find(deviceId);
                if (ii != m_configurations.end()) {
                    ii->second.set("use", useFlag);
                    vector<Hash> expert = (ii->second.has("expert") ? ii->second.get<vector < Hash >> ("expert") : vector < Hash >());
                    for (vector<Hash>::iterator jj = expert.begin(); jj != expert.end(); ++jj) {
                        jj->set("use", useFlag);
                    }

                    vector<Hash> user = (ii->second.has("user") ? ii->second.get<vector < Hash >> ("user") : vector < Hash >());
                    for (vector<Hash>::iterator jj = user.begin(); jj != user.end(); ++jj) {
                        jj->set("use", useFlag);
                    }
                }
            }
        }


        void RunConfigurator::reconfigureSourcesOnly(const std::vector<karabo::util::Hash>& sources, const karabo::util::Schema & schema) {
        }


        void RunConfigurator::reconfigureSources(const std::vector<karabo::util::Hash>& sources, const karabo::util::Schema & schema) {

            for (vector<Hash>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
                const Hash& hash = *it;
                const string& source = hash.get<string>("source");
                const bool useFlag = hash.get<bool>("use");
                const string& behavior = hash.get<string>("behavior");
                const bool monitored = hash.get<bool>("monitored");

                bool found = false;

                vector<Hash>& expert = m_configurations[m_cursor].get<vector < Hash >> ("expert");
                for (vector<Hash>::iterator ii = expert.begin(); ii != expert.end(); ++ii) {
                    if (source == ii->get<string>("source")) {
                        found = true;
                        ii->set("use", useFlag);
                        ii->set("behavior", behavior);
                        ii->set("monitored", monitored);
                        break;
                    }
                }

                if (found) continue;

                vector<Hash>& user = m_configurations[m_cursor].get<vector < Hash >> ("user");
                for (vector<Hash>::iterator ii = user.begin(); ii != user.end(); ++ii) {
                    if (source == ii->get<string>("source")) {
                        found = true;
                        ii->set("use", useFlag);
                        ii->set("behavior", behavior);
                        ii->set("monitored", monitored);
                        break;
                    }
                }
            }
        }
    }
}
