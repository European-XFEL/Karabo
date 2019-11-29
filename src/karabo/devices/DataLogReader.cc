#include <map>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <streambuf>

#include "karabo/io/Input.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/util/TimeDuration.hh"
#include "karabo/util/Version.hh"

#include "DataLogReader.hh"


namespace karabo {
    namespace devices {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        DataLogReader::DataLogReader(const Hash& input)
            : karabo::core::Device<karabo::core::OkErrorFsm>(input) {
            m_serializer = TextSerializer<Hash>::create("Xml");
            m_schemaSerializer = TextSerializer<Schema>::create("Xml");
            KARABO_SLOT(slotGetPropertyHistory, string /*deviceId*/, string /*key*/, Hash /*params*/);
            KARABO_SLOT(slotGetConfigurationFromPast, string /*deviceId*/, string /*timepoint*/);
        }


        DataLogReader::~DataLogReader() {
            KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << " being destructed.";
        }


        void DataLogReader::okStateOnEntry() {
        }

    } // namespace devices

} // namespace karabo
