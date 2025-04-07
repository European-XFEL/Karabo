/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "DataLogReader.hh"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <sstream>
#include <streambuf>
#include <vector>

#include "karabo/data/io/FileTools.hh"
#include "karabo/data/io/Input.hh"
#include "karabo/data/time/TimeDuration.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/Version.hh"


namespace karabo {
    namespace devices {

        using namespace std;
        using namespace karabo::data;


        void DataLogReader::expectedParameters(Schema& expected) {
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


        DataLogReader::DataLogReader(const Hash& input) : karabo::core::Device(input) {
            m_visibility = karabo::data::Schema::ADMIN;
            KARABO_INITIAL_FUNCTION(initialize)
            KARABO_SLOT(slotGetPropertyHistory, string /*deviceId*/, string /*key*/, Hash /*params*/);
            KARABO_SLOT(slotGetConfigurationFromPast, string /*deviceId*/, string /*timepoint*/);
        }


        DataLogReader::~DataLogReader() {
            KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << " being destructed.";
        }


        void DataLogReader::slotGetPropertyHistory(const std::string& deviceId, const std::string& property,
                                                   const karabo::data::Hash& params) {
            set("numGetPropertyHistory", get<unsigned int>("numGetPropertyHistory") + 1);

            slotGetPropertyHistoryImpl(deviceId, property, params);
        }


        void DataLogReader::slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint) {
            set("numGetConfigurationFromPast", get<unsigned int>("numGetConfigurationFromPast") + 1);

            slotGetConfigurationFromPastImpl(deviceId, timepoint);
        }

        void DataLogReader::initialize() {
            onOk();
        }

        void DataLogReader::onOk() {
            if (getState() != State::ON) {
                updateState(State::ON);
            }
        }

        const string DataLogReader::onException(const string& message) {
            ostringstream oss;
            try {
                throw;
            } catch (const std::exception& e) {
                oss << message << " : " << e.what();
            }
            KARABO_LOG_FRAMEWORK_ERROR << oss.str();
            updateState(State::ERROR, Hash("status", message));
            return oss.str();
        }

    } // namespace devices

} // namespace karabo
