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
/*
 * File:   PipeReceiverDevice.cc
 * Author: flucke
 *
 * Created on October 14, 2016, 2:30 PM
 */
#include "PipeReceiverDevice.hh"

#include <chrono>
#include <thread>

#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/NDArray.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/data/types/StringTools.hh"
#include "karabo/data/types/Units.hh"
#include "karabo/xms/InputChannel.hh"

namespace karabo {

    using data::BOOL_ELEMENT;
    using data::FLOAT_ELEMENT;
    using data::INT32_ELEMENT;
    using data::NDArray;
    using data::toString;
    using data::UINT32_ELEMENT;
    using data::VECTOR_STRING_ELEMENT;
    using xms::INPUT_CHANNEL;

    KARABO_REGISTER_FOR_CONFIGURATION(core::BaseDevice, core::Device, PipeReceiverDevice)

    void PipeReceiverDevice::expectedParameters(data::Schema& expected) {
        data::Schema data;
        INT32_ELEMENT(data).key("dataId").readOnly().commit();

        INPUT_CHANNEL(expected)
              .key("input")
              .displayedName("Input")
              .description("Input channel: client")
              .dataSchema(data)
              .commit();

        INPUT_CHANNEL(expected).key("input2").displayedName("Input2").description("Input channel: client").commit();

        BOOL_ELEMENT(expected)
              .key("onData")
              .displayedName("Use callback interface onData")
              .description("If false, use callback per InputChannel, not per Data")
              .assignmentOptional()
              .defaultValue(false)
              .commit();


        UINT32_ELEMENT(expected)
              .key("processingTime")
              .displayedName("Processing Time")
              .description("Simulated processing time")
              .assignmentOptional()
              .defaultValue(0)
              .unit(data::Unit::SECOND)
              .metricPrefix(data::MetricPrefix::MILLI)
              .reconfigurable()
              .commit();

        UINT32_ELEMENT(expected)
              .key("currentDataId")
              .displayedName("Current Data ID")
              .description("Monitors the currently processed data token")
              .readOnly()
              .commit();

        UINT32_ELEMENT(expected)
              .key("dataItemSize")
              .displayedName("Data element size")
              .description("Data element size in bytes.")
              .readOnly()
              .commit();

        UINT32_ELEMENT(expected)
              .key("nTotalData")
              .displayedName("Total number of data tokens")
              .description("The total number of data received within one stream")
              .readOnly()
              .initialValue(0u)
              .commit();

        UINT32_ELEMENT(expected)
              .key("nTotalDataOnEos")
              .displayedName("Total data on EOS")
              .description("The total number of data received when End of Stream was received")
              .readOnly()
              .initialValue(0u)
              .commit();

        VECTOR_STRING_ELEMENT(expected).key("dataSources").displayedName("Data sources on input").readOnly().commit();

        VECTOR_STRING_ELEMENT(expected)
              .key("dataSourcesFromIndex")
              .displayedName("Data sources on input from index resolve")
              .readOnly()
              .commit();

        FLOAT_ELEMENT(expected).key("averageTransferTime").readOnly().commit();
    }


    PipeReceiverDevice::PipeReceiverDevice(const karabo::data::Hash& config) : Device(config) {
        KARABO_SLOT0(reset);
        KARABO_INITIAL_FUNCTION(initialization)
    }


    void PipeReceiverDevice::initialization() {
        if (get<bool>("onData")) {
            KARABO_ON_DATA("input", onData);
        } else {
            KARABO_ON_INPUT("input", onInput);
        }
        KARABO_ON_INPUT("input2", onInputProfile);
        KARABO_ON_EOS("input", onEndOfStream);
        KARABO_ON_EOS("input2", onEndOfStreamProfile);
    }


    void PipeReceiverDevice::onInput(const xms::InputChannel::Pointer& input) {
        set("dataSources", input->getMetaData()[0].getSource());
        std::vector<std::string> sources;
        data::Hash data;
        for (size_t i = 0; i < input->size(); ++i) {
            input->read(data, i); // calls Memory:read, which calls data.clear() before filling it
            sources.push_back(input->indexToMetaData(i).getSource());
            onData(data, input->indexToMetaData(i));
        }
        set("dataSourcesFromIndex", sources);
    }


    void PipeReceiverDevice::onData(const data::Hash& data, const xms::InputChannel::MetaData& metaData) {
        set("dataSources", std::vector<std::string>(1, metaData.getSource()));
        set("currentDataId", data.get<int>("dataId"));
        const auto& v = data.get<std::vector<long long>>("data");
        unsigned int bytes = v.size() * sizeof(long long);
        set<unsigned int>("dataItemSize", bytes);
        const auto& emptyArr = data.get<NDArray>("emptyArray");
        if (emptyArr.size() != 0) {
            std::string status = get<std::string>("status");
            if (!status.empty()) status += "\n";
            set("status",
                status += "dataId " + toString(data.get<int>("dataId")) += " has size " + toString(emptyArr.size()));
        }

        // Sum total number of data
        set("nTotalData", get<unsigned int>("nTotalData") + 1);
        unsigned int processingTime = get<unsigned int>("processingTime");
        if (processingTime > 0) std::this_thread::sleep_for(std::chrono::milliseconds(processingTime));
    }


    void PipeReceiverDevice::onEndOfStream(const xms::InputChannel::Pointer& input) {
        set<unsigned int>("nTotalDataOnEos", get<unsigned int>("nTotalData"));
    }

    void PipeReceiverDevice::onInputProfile(const xms::InputChannel::Pointer& input) {
        using namespace std::chrono;
        data::Hash data;

        auto microseconds_today = []() -> unsigned long long {
            auto now = system_clock::now();
            return round<microseconds>(now - floor<days>(now)).count();
        };

        for (size_t i = 0; i < input->size(); ++i) {
            input->read(data, i); // clears data before filling
            unsigned long long transferTime = microseconds_today() - data.get<unsigned long long>("inTime");
            m_transferTimes.push_back(transferTime);
            set("nTotalData", get<unsigned int>("nTotalData") + 1);
            const karabo::data::NDArray& arr = data.get<karabo::data::NDArray>("array");
            KARABO_LOG_INFO << arr.byteSize();
        }
    }


    void PipeReceiverDevice::onEndOfStreamProfile(const xms::InputChannel::Pointer& input) {
        unsigned long long transferTime = 0;
        for (auto time = m_transferTimes.begin(); time != m_transferTimes.end(); ++time) {
            transferTime += *time;
        }

        set<float>("averageTransferTime", (float)transferTime / m_transferTimes.size());
    }

    void PipeReceiverDevice::reset() {
        m_transferTimes.clear();

        set(karabo::data::Hash("nTotalData", 0u, "nTotalDataOnEos", 0u, "averageTransferTime", 0.f));
    }

} // namespace karabo
