#include "RunConfigurationGroup.hh"
#include "karabo/util/GenericElement.hh"
#include "karabo/util/LeafElement.hh"
#include "karabo/util/ChoiceElement.hh"
#include "karabo/xms/SlotElement.hh"
#include "karabo/core/Device.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::xms;

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
                    .options("init,read-only,record-all")
                    .assignmentOptional().defaultValue("record-all")
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
                    .assignmentMandatory()
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

            SLOT_ELEMENT(expected).key("group.saveGroupConfiguration")
                    .displayedName("Save configuration")
                    .description("Push the button to save configuration in 'run_config_group' folder.")
                    .commit();
        }


        RunConfigurationGroup::RunConfigurationGroup(const karabo::util::Hash& input) : Device(input) {

            KARABO_INITIAL_FUNCTION(initialize);

            KARABO_SYSTEM_SIGNAL("signalGetGroup", string, Hash);
            KARABO_SLOT(slotGetGroup);
            KARABO_SLOT(saveGroupConfiguration);
        }


        RunConfigurationGroup::~RunConfigurationGroup() {
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


        void RunConfigurationGroup::saveGroupConfiguration() {
            karabo::io::saveToFile<Hash>(get<Hash>("group"), "run_config_groups/" + getInstanceId() + ".xml");
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

                const vector<Hash>& current = (currentGroup.has("expert") ? currentGroup.get<vector < Hash >> ("expert") : std::vector<Hash>());
                vector<Hash>& input = inputGroup.get<vector < Hash >> ("expert");
                fillTable(current, input, expert);
                // update expert table
                inputGroup.set("expert", expert);
            }

            if (inputGroup.has("user")) {
                vector<Hash> user;

                const vector<Hash>& current = (currentGroup.has("user") ? currentGroup.get<vector < Hash >> ("user") : std::vector<Hash>());
                vector<Hash>& input = inputGroup.get<vector < Hash >> ("user");
                fillTable(current, input, user);
                // set new version of expert table
                inputGroup.set("user", user);
            }
        }
        


        void RunConfigurationGroup::fillTable(const std::vector<karabo::util::Hash>& current,
                                              std::vector<karabo::util::Hash>& input,
                                              std::vector<karabo::util::Hash>& table) {

            for (size_t i = 0; i < input.size(); i++) {
                Hash& hash = input[i];

                const string& deviceId = hash.get<string>("source");

                if (deviceId.find_first_of(OUTPUT_CHANNEL_SEPARATOR) == std::string::npos)
                    hash.setAttribute<bool>("source", "pipeline", false);
                else
                    hash.setAttribute<bool>("source", "pipeline", true);

                const bool pipeline = hash.getAttribute<bool>("source", "pipeline");
                table.push_back(hash);
                vector<Hash>::const_iterator it = findDataSource(current, deviceId);
                if (!pipeline) {
                    // Device ....
                    if (it == current.end()) {
                        // ... is new one ... get its output channels and add them to the table ...
                        vector<string> ochannels = remote().getOutputChannelNames(deviceId);
                        for (size_t ch = 0; ch < ochannels.size(); ch++) {
                            Hash row("source", deviceId + OUTPUT_CHANNEL_SEPARATOR + ochannels[ch],
                                     "type", "control",
                                     "behavior", "read-only",
                                     "monitored", false);
                            row.setAttribute("source", "pipeline", true);
                            table.push_back(row);
                        }
                    } else {
                        // ... exists. Check if its output channels are in the table and ...
                        // ... if not, add them to the table ...
                        vector<string> ochannels = remote().getOutputChannelNames(deviceId);
                        for (size_t ch = 0; ch < ochannels.size(); ch++) {
                            vector<Hash>::const_iterator ii = findDataSource(current, deviceId + OUTPUT_CHANNEL_SEPARATOR + ochannels[ch]);
                            if (ii == current.end()) {
                                //Existing device has output channel not inserted yet
                                Hash row("source", deviceId + OUTPUT_CHANNEL_SEPARATOR + ochannels[ch],
                                                     "type", "control",
                                                     "behavior", "read-only",
                                                     "monitored", false);
                                row.setAttribute("source", "pipeline", true);
                                table.push_back(row);
                            }
                        }
                    }
                }
            }
        }
    }
}
