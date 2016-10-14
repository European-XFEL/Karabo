#include "RunConfigurationGroup.hh"
#include "karabo/util/GenericElement.hh"
#include "karabo/util/LeafElement.hh"
#include "karabo/util/ChoiceElement.hh"
#include "karabo/core/Device.hh"

using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace devices {


        KARABO_REGISTER_FOR_CONFIGURATION(RunControlDataSource)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, RunConfigurationGroup)

        void RunControlDataSource::expectedParameters(karabo::util::Schema& expected) {

            STRING_ELEMENT(expected).key("source")
                    .displayedName("Source")
                    .description("Data source's full name, like SASE1/SPB/SAMP/INJ_CAM_1")
                    .assignmentOptional().defaultValue("Source")
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(expected).key("pipeline")
                    .displayedName("Pipeline flag")
                    .description("Flag indicating the source as P2P output channel")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();

            STRING_ELEMENT(expected).key("type")
                    .displayedName("Type")
                    .description("Data source's type")
                    //.options("control, instrument")
                    .assignmentOptional().defaultValue("control")
                    .reconfigurable()
                    .commit();

            STRING_ELEMENT(expected).key("behavior")
                    .displayedName("Behavior")
                    .description("Configure data source's behavior")
                    .options("ignore,read-only,record-all")
                    .assignmentOptional().defaultValue("ignore")
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(expected).key("monitored")
                    .displayedName("Monitor out")
                    .description("If true, the selected data will be output to the online pipeline outputs in the DAQ's monitoring and recording states.")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();

        }


        RunControlDataSource::RunControlDataSource(const karabo::util::Hash& input) {

            if (input.empty() || !input.has("source") || input.get<string>("source").empty())
                throw KARABO_PARAMETER_EXCEPTION("Invalid input configuration ....\n" + toString(input));
        }


        RunControlDataSource::~RunControlDataSource() {
        }


        void RunControlDataSource::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
            KARABO_LOG_FRAMEWORK_INFO << "RunControlDataSource::preReconfigure  .... incomingReconfiguration ==> ...\n" << incomingReconfiguration;
        }


        void RunConfigurationGroup::expectedParameters(karabo::util::Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State::INIT, State::NORMAL, State::ERROR)
                    .setNewDefaultValue(State::INIT)
                    .commit();

            NODE_ELEMENT(expected).key("group")
                    .displayedName("Group")
                    .description("Structure describing data sources logically belonging together.")
                    .commit();

            STRING_ELEMENT(expected).key("group.id")
                    .displayedName("Name")
                    .description("Name of run configuration group.")
                    .assignmentOptional().defaultValue("Enter group id")
                    .reconfigurable()
                    .commit();

            STRING_ELEMENT(expected).key("group.description")
                    .displayedName("Description")
                    .description("Description of current run configuration group.")
                    .assignmentOptional().noDefaultValue()
                    .reconfigurable()
                    .commit();

            TABLE_ELEMENT(expected).key("group.expert")
                    .displayedName("Mandatory sources")
                    .description("Expert configurations for mandatory data sources")
                    .addColumnsFromClass<RunControlDataSource>()
                    .assignmentOptional().noDefaultValue()
                    .reconfigurable()
                    .commit();

            TABLE_ELEMENT(expected).key("group.user")
                    .displayedName("Optional sources")
                    .description("User selectable data sources.")
                    .addColumnsFromClass<RunControlDataSource>()
                    .assignmentOptional().noDefaultValue()
                    .reconfigurable()
                    .commit();
        }


        RunConfigurationGroup::RunConfigurationGroup(const karabo::util::Hash& input) : Device(input) {

            KARABO_INITIAL_FUNCTION(initialize);

            KARABO_SYSTEM_SIGNAL("signalGetGroup", string, Hash);
            KARABO_SLOT(slotGetGroup);
        }


        RunConfigurationGroup::~RunConfigurationGroup() {
            karabo::io::saveToFile<Hash>(get<Hash>("group"), "run_config_groups/" + getInstanceId() + ".xml");
        }


        void RunConfigurationGroup::initialize() {
            Hash group;
            string filename("run_config_groups/" + getInstanceId() + ".xml");
            if (!boost::filesystem::exists("run_config_groups"))
                boost::filesystem::create_directory("run_config_groups");
            else if (boost::filesystem::exists(filename))
                karabo::io::loadFromFile(group, filename);

            set("group", group);

            updateState(State::NORMAL);
        }


        void RunConfigurationGroup::slotGetGroup() {
            emit("signalGetGroup", getInstanceId(), getGroup());
        }


        static const std::vector<karabo::util::Hash>::const_iterator& findDataSource(const std::vector<karabo::util::Hash>& vec, const std::string& id) {
            for (vector<Hash>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
                if (it->get<string>("source") == id) return it;
            }
            return vec.end();
        }


        void RunConfigurationGroup::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
            KARABO_LOG_FRAMEWORK_DEBUG << "RunConfigurationGroup::preReconfigure  .... incomingReconfiguration ==> ...\n" << incomingReconfiguration;

            if (!incomingReconfiguration.has("group")) return;

            Hash& inputGroup = incomingReconfiguration.get<Hash>("group");
            const Hash& currentGroup = get<Hash>("group");

            if (inputGroup.has("expert")) {
                vector<Hash> expert;

                const vector<Hash>& current = currentGroup.get<vector < Hash >> ("expert");
                const vector<Hash>& input = inputGroup.get<vector < Hash >> ("expert");
                fillTable(current, input, expert);
                // update expert table
                inputGroup.set("expert", expert);
            }

            if (inputGroup.has("user")) {
                vector<Hash> user;

                const vector<Hash>& current = currentGroup.get<vector < Hash >> ("user");
                const vector<Hash>& input = inputGroup.get<vector < Hash >> ("user");
                fillTable(current, input, user);
                // set new version of expert table
                inputGroup.set("user", user);
            }
        }


        void RunConfigurationGroup::fillTable(const std::vector<karabo::util::Hash>& current,
                                              const std::vector<karabo::util::Hash>& input,
                                              std::vector<karabo::util::Hash>& table) {

            for (size_t i = 0; i < input.size(); i++) {
                const string& deviceId = input[i].get<string>("source");
                const bool pipeline = input[i].get<bool>("pipeline");
                table.push_back(input[i]);
                vector<Hash>::const_iterator it = findDataSource(current, deviceId);
                if (!pipeline) {
                    // Device ....
                    if (it == current.end()) {
                        // ... is new one ... get its output channels and add them to the table ...
                        vector<string> ochannels = remote().getOutputChannelNames(deviceId);
                        for (size_t ch = 0; ch < ochannels.size(); ch++) {
                            table.push_back(Hash("source", deviceId + ":" + ochannels[ch],
                                                 "pipeline", true,
                                                 "type", "control",
                                                 "behavior", "ignore",
                                                 "monitored", false));
                        }
                    } else {
                        // ... exists. Check if its output channels are in the table and ...
                        // ... if not, add them to the table ...
                        vector<string> ochannels = remote().getOutputChannelNames(deviceId);
                        for (size_t ch = 0; ch < ochannels.size(); ch++) {
                            vector<Hash>::const_iterator ii = findDataSource(current, deviceId + ":" + ochannels[ch]);
                            if (ii == current.end()) {
                                //Existing device has output channel not inserted yet
                                table.push_back(Hash("source", deviceId + ":" + ochannels[ch],
                                                     "pipeline", true,
                                                     "type", "control",
                                                     "behavior", "ignore",
                                                     "monitored", false));
                            }
                        }
                    }
                }
            }
        }
    }
}
