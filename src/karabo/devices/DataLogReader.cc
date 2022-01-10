#include "DataLogReader.hh"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <sstream>
#include <streambuf>
#include <vector>

#include "karabo/io/FileTools.hh"
#include "karabo/io/Input.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/TimeDuration.hh"
#include "karabo/util/Version.hh"


namespace karabo {
    namespace devices {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        void DataLogReader::expectedParameters(Schema& expected) {
            OVERWRITE_ELEMENT(expected).key("visibility").setNewDefaultValue<int>(Schema::AccessLevel::ADMIN).commit();

            UINT32_ELEMENT(expected)
                  .key("numGetPropertyHistory")
                  .displayedName("N(get history)")
                  .description("Number of calls to slotGetPropertyHistory")
                  .unit(UnitType::COUNT)
                  .readOnly()
                  .initialValue(0u)
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("numGetConfigurationFromPast")
                  .displayedName("N(get config)")
                  .description("Number of calls to slotGetConfigurationFromPast")
                  .unit(UnitType::COUNT)
                  .readOnly()
                  .initialValue(0u)
                  .commit();
        }


        DataLogReader::DataLogReader(const Hash& input) : karabo::core::Device<karabo::core::OkErrorFsm>(input) {
            KARABO_SLOT(slotGetPropertyHistory, string /*deviceId*/, string /*key*/, Hash /*params*/);
            KARABO_SLOT(slotGetConfigurationFromPast, string /*deviceId*/, string /*timepoint*/);
        }


        DataLogReader::~DataLogReader() {
            KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << " being destructed.";
        }


        void DataLogReader::slotGetPropertyHistory(const std::string& deviceId, const std::string& property,
                                                   const karabo::util::Hash& params) {
            set("numGetPropertyHistory", get<unsigned int>("numGetPropertyHistory") + 1);

            slotGetPropertyHistoryImpl(deviceId, property, params);
        }


        void DataLogReader::slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint) {
            set("numGetConfigurationFromPast", get<unsigned int>("numGetConfigurationFromPast") + 1);

            slotGetConfigurationFromPastImpl(deviceId, timepoint);
        }

        void DataLogReader::okStateOnEntry() {}

    } // namespace devices

} // namespace karabo
