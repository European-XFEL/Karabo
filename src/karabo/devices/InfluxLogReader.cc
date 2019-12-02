/*
 * File:   InfluxLogReader.cc
 * Author: <raul.costa@xfel.eu>
 *
 * Created on November 8, 2019, 3:40 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <map>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <streambuf>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "karabo/core/Device.hh"
#include "karabo/io/Input.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/TimeDuration.hh"
#include "karabo/util/Version.hh"

#include "DataLogReader.hh"
#include "InfluxLogReader.hh"

namespace bf = boost::filesystem;
namespace bs = boost::system;

namespace karabo {
    namespace devices {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice,
                                          karabo::core::Device<karabo::core::OkErrorFsm>,
                                          DataLogReader,
                                          InfluxLogReader)


        void InfluxLogReader::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue<int>(Schema::AccessLevel::ADMIN)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("archive")
                    .setNewDefaultValue(false)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();

            PATH_ELEMENT(expected).key("url")
                    .displayedName("Url")
                    .description("")
                    .assignmentOptional().defaultValue("tcp://localhost:8086")
                    .commit();
        }


        InfluxLogReader::InfluxLogReader(const Hash& input)
            : karabo::devices::DataLogReader(input) {
        }


        InfluxLogReader::~InfluxLogReader() {
            KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << " being destructed.";
        }


        void InfluxLogReader::slotGetPropertyHistory(const std::string& deviceId,
                                                   const std::string& property,
                                                   const Hash& params) {
        }


        void InfluxLogReader::slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint) {
        }

    } // namespace devices

} // namespace karabo

