/* 
 * File:   PipeReceiverDevice.cc
 * Author: flucke
 * 
 * Created on October 14, 2016, 2:30 PM
 */

#include "PipeReceiverDevice.hh"

#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/Units.hh"
#include "karabo/xms/InputChannel.hh"

#include "karabo/util/SimpleElement.hh"
#include "karabo/xms/InputChannel.hh"

namespace karabo {

    using util::BOOL_ELEMENT;
    using util::INT32_ELEMENT;
    using util::UINT32_ELEMENT;
    using util::VECTOR_STRING_ELEMENT;
    using xms::INPUT_CHANNEL;
    using util::FLOAT_ELEMENT;
    
    KARABO_REGISTER_FOR_CONFIGURATION(core::BaseDevice, core::Device<>, PipeReceiverDevice)

    void PipeReceiverDevice::expectedParameters(util::Schema& expected) {

        util::Schema data;
        INT32_ELEMENT(data).key("dataId")
                .readOnly()
                .commit();

        INPUT_CHANNEL(expected).key("input")
                .displayedName("Input")
                .description("Input channel: client")
                .dataSchema(data)
                .commit();
        
        INPUT_CHANNEL(expected).key("input2")
                .displayedName("Input2")
                .description("Input channel: client")
                .commit();

        BOOL_ELEMENT(expected).key("onData")
                .displayedName("Use callback interface onData")
                .description("If false, use callback per InputChannel, not per Data")
                .assignmentOptional().defaultValue(false)
                .commit();


        UINT32_ELEMENT(expected).key("processingTime")
                .displayedName("Processing Time")
                .description("Simulated processing time")
                .assignmentOptional().defaultValue(0)
                .unit(util::Unit::SECOND)
                .metricPrefix(util::MetricPrefix::MILLI)
                .reconfigurable()
                .commit();

        UINT32_ELEMENT(expected).key("currentDataId")
                .displayedName("Current Data ID")
                .description("Monitors the currently processed data token")
                .readOnly()
                .commit();

        UINT32_ELEMENT(expected).key("dataItemSize")
                .displayedName("Data element size")
                .description("Data element size in bytes.")
                .readOnly()
                .commit();

        UINT32_ELEMENT(expected).key("nTotalData")
                .displayedName("Total number of data tokens")
                .description("The total number of data received within one stream")
                .readOnly()
                .initialValue(0u)
                .commit();

        UINT32_ELEMENT(expected).key("nTotalOnEos")
                .displayedName("Total on Eos ")
                .description("The total number of data received when End of Stream was received")
                .readOnly()
                .initialValue(0u)
                .commit();
        
        VECTOR_STRING_ELEMENT(expected).key("dataSources")
                .displayedName("Data sources on input")
                .readOnly()
                .commit();
        
        VECTOR_STRING_ELEMENT(expected).key("dataSourcesFromIndex")
                .displayedName("Data sources on input from index resolve")
                .readOnly()
                .commit();
        
        FLOAT_ELEMENT(expected).key("averageTransferTime")
                .readOnly()
                .commit();

    }


    PipeReceiverDevice::PipeReceiverDevice(const karabo::util::Hash& config) : Device<>(config) {

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
        util::Hash data;
        for (size_t i = 0; i < input->size(); ++i) {
            input->read(data, i); // calls Memory:read, which calls data.clear() before filling it
            sources.push_back(input->indexToMetaData(i).getSource());
            onData(data, input->indexToMetaData(i));
        }
        set("dataSourcesFromIndex", sources);
    }


    void PipeReceiverDevice::onData(const util::Hash& data, const xms::InputChannel::MetaData& metaData) {
        set("dataSources", std::vector<std::string>(1, metaData.getSource()));
        set("currentDataId", data.get<int>("dataId"));
        const auto v = data.get<std::vector<long long>>("data");
        unsigned int bytes = v.size() * sizeof(long long);
        set<unsigned int>("dataItemSize", bytes);

        // Sum total number of data
        set("nTotalData", get<unsigned int>("nTotalData") + 1);
        boost::this_thread::sleep(boost::posix_time::milliseconds(get<unsigned int>("processingTime")));
    }


    void PipeReceiverDevice::onEndOfStream(const xms::InputChannel::Pointer& input) {

        set<unsigned int>("nTotalOnEos", get<unsigned int>("nTotalData"));
    }

    void PipeReceiverDevice::onInputProfile(const xms::InputChannel::Pointer& input) {
        util::Hash data;
        
        for (size_t i = 0; i < input->size(); ++i) {
            input->read(data, i); // clears data before filling        
            unsigned long long transferTime = boost::posix_time::microsec_clock::local_time().time_of_day().total_microseconds() - data.get<unsigned long long>("inTime");
            m_transferTimes.push_back(transferTime);
            set("nTotalData", get<unsigned int>("nTotalData") + 1);
            karabo::util::NDArray arr = data.get<karabo::util::NDArray>("array");
            KARABO_LOG_INFO<<arr.byteSize();
        }
         
    }


    void PipeReceiverDevice::onEndOfStreamProfile(const xms::InputChannel::Pointer& input) {
        unsigned long long transferTime = 0;
        for(auto time = m_transferTimes.begin(); time != m_transferTimes.end(); ++time) {
            transferTime += *time;
        }

        set<float>("averageTransferTime", (float)transferTime/m_transferTimes.size());
    }
    
    void PipeReceiverDevice::reset() {
        m_transferTimes.clear();

        set(karabo::util::Hash("nTotalData", 0u,
                               "nTotalOnEos", 0u,
                               "averageTransferTime", 0.f));
    }

}

