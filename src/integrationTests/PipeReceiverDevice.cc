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
    using xms::INPUT_CHANNEL_ELEMENT;

    KARABO_REGISTER_FOR_CONFIGURATION(core::BaseDevice, core::Device<>, PipeReceiverDevice)

    void PipeReceiverDevice::expectedParameters(util::Schema& expected) {

        util::Schema data;
        INT32_ELEMENT(data).key("dataId")
                .readOnly()
                .commit();

        INPUT_CHANNEL_ELEMENT(expected).key("input")
                .displayedName("Input")
                .description("Input channel: client")
                .dataSchema(data)
                .commit();

        BOOL_ELEMENT(expected).key("onData")
                .displayedName("Use callback interface onData")
                .description("If false, use callback per InputChannel, not per Data")
                .assignmentOptional().defaultValue(true)
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

        UINT32_ELEMENT(expected).key("nTotalData")
                .displayedName("Total number of data tokens")
                .description("The total number of data received within one stream")
                .readOnly()
                .commit();

        UINT32_ELEMENT(expected).key("nTotalOnEos")
                .displayedName("Total on Eos ")
                .description("The total number of data received when End of Stream was received")
                .readOnly()
                .commit();

    }


    PipeReceiverDevice::PipeReceiverDevice(const karabo::util::Hash& config) : Device<>(config) {

        KARABO_INITIAL_FUNCTION(initialization)
    }


    void PipeReceiverDevice::initialization() {

        if (get<bool>("onData")) {
            KARABO_ON_DATA("input", onData);
        } else {
            KARABO_ON_INPUT("input", onInput);
        }
        KARABO_ON_EOS("input", onEndOfStream);

    }


    void PipeReceiverDevice::onInput(const xms::InputChannel::Pointer& input) {

        util::Hash data;
        for (size_t i = 0; i < input->size(); ++i) {
            input->read(data, i); // clears data before filling
            onData(data);
        }
    }


    void PipeReceiverDevice::onData(const util::Hash& data) {

        set("currentDataId", data.get<int>("dataId"));

        // Sum total number of data
        set("nTotalData", get<unsigned int>("nTotalData") + 1);
        boost::this_thread::sleep(boost::posix_time::milliseconds(get<unsigned int>("processingTime")));
    }


    void PipeReceiverDevice::onEndOfStream(const xms::InputChannel::Pointer& input) {

        set<unsigned int>("nTotalOnEos", get<unsigned int>("nTotalData"));
    }

}

