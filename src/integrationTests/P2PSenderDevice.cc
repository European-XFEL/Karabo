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
 * File:   P2PSenderDeviceDevice.cc
 * Author: haufs
 *
 * Created on September 20, 2016, 3:49 PM
 */

#include "P2PSenderDevice.hh"

#include <chrono>
#include <thread>

#include "karabo/data/schema/NDArrayElement.hh"

using namespace std::chrono;
using namespace std;

using std::placeholders::_1;

USING_KARABO_NAMESPACES;

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device, P2PSenderDevice)

    void P2PSenderDevice::expectedParameters(Schema& expected) {
        OVERWRITE_ELEMENT(expected)
              .key("state")
              .setNewOptions(State::NORMAL, State::ACTIVE)
              .setNewDefaultValue(State::NORMAL)
              .commit();

        SLOT_ELEMENT(expected)
              .key("write")
              .displayedName("Write")
              .description("Write some data")
              .allowedStates(State::NORMAL)
              .commit();

        SLOT_ELEMENT(expected)
              .key("stop")
              .displayedName("Stop")
              .description("Stop writing data")
              .allowedStates(State::ACTIVE)
              .commit();

        Schema data;
        INT32_ELEMENT(data).key("dataId").readOnly().commit();

        STRING_ELEMENT(data).key("sha1").readOnly().commit();

        STRING_ELEMENT(data).key("flow").readOnly().commit();

        VECTOR_INT64_ELEMENT(data).key("data").readOnly().commit();

        NDARRAY_ELEMENT(data).key("array").dtype(Types::DOUBLE).shape("100,200,0").commit();

        OUTPUT_CHANNEL(expected).key("output1").displayedName("Output1").dataSchema(data).commit();

        Schema data2;

        UINT64_ELEMENT(data2).key("inTime").readOnly().commit();

        NDARRAY_ELEMENT(data2).key("array").dtype(Types::DOUBLE).shape("256,256,512").commit();

        OUTPUT_CHANNEL(expected).key("output2").displayedName("Output2").dataSchema(data2).commit();


        UINT32_ELEMENT(expected)
              .key("nData")
              .displayedName("Number of data")
              .description("Number of data")
              .assignmentOptional()
              .defaultValue(12)
              .reconfigurable()
              .commit();

        UINT32_ELEMENT(expected)
              .key("delay")
              .displayedName("Delay")
              .description("Delay between writes")
              .assignmentOptional()
              .defaultValue(0u)
              .unit(Unit::SECOND)
              .metricPrefix(MetricPrefix::MILLI)
              .reconfigurable()
              .commit();

        UINT32_ELEMENT(expected)
              .key("currentDataId")
              .displayedName("Current Data ID")
              .description("Monitors the currently processed data token")
              .readOnly()
              .commit();

        STRING_ELEMENT(expected)
              .key("scenario")
              .options("test,profile")
              .assignmentOptional()
              .defaultValue("test")
              .reconfigurable()
              .commit();

        UINT32_ELEMENT(expected)
              .key("dataSize")
              .description("Size of the INT64 'data' vector sent in 'test' scenario")
              .assignmentOptional()
              .defaultValue(1000000u)
              .reconfigurable()
              .commit();

        BOOL_ELEMENT(expected).key("safeNDArray").assignmentOptional().defaultValue(true).reconfigurable().commit();

        STRING_ELEMENT(expected)
              .key("nextSharedInput")
              .description(
                    "An input channel id to register at 'output1' for non-load-balanced shared distribution. "
                    "Empty string means reset such a handler.")
              .assignmentOptional()
              .defaultValue(std::string())
              .reconfigurable()
              .commit();
    }


    P2PSenderDevice::P2PSenderDevice(const Hash& config) : Device(config), m_stopWriting(true) {
        KARABO_SLOT0(write);
        KARABO_SLOT0(stop);

        KARABO_INITIAL_FUNCTION(initialize);
    }


    P2PSenderDevice::~P2PSenderDevice() {
        if (m_writingThread.joinable()) {
            m_writingThread.join();
        }
    }

    void P2PSenderDevice::initialize() {
        Hash cfgCopy(getCurrentConfiguration());
        preReconfigure(cfgCopy);
    }

    void P2PSenderDevice::preReconfigure(Hash& incomingCfg) {
        boost::optional<Hash::Node&> nextSharedInputNode = incomingCfg.find("nextSharedInput");
        if (nextSharedInputNode) {
            const std::string& nextSharedInput = nextSharedInputNode->getValue<std::string>();
            xms::SharedInputSelector selector; // empty function pointer
            if (nextSharedInput == "returnEmptyString") {
                // test with lambda...
                selector = [](const std::vector<std::string>&) { return std::string(); };
            } else if (nextSharedInput == "roundRobinSelector") {
                auto counter = make_shared<size_t>(0);
                selector = [counter](const std::vector<std::string>& inputs) mutable {
                    const size_t iSize = inputs.size();
                    if (iSize > 0) {
                        const size_t index = (*counter)++ % iSize;
                        return inputs[index];
                    } else {
                        return std::string();
                    }
                };
            } else if (!nextSharedInput.empty()) {
                // ...and test with bind_weak of member function
                selector = bind_weak(&P2PSenderDevice::selectSharedInput, this, nextSharedInput, _1);
            }
            // set new selector (or unset selection if nextSharedInput empty)
            getOutputChannel("output1")->registerSharedInputSelector(std::move(selector));
        }
    }

    std::string P2PSenderDevice::selectSharedInput(const std::string& result, const std::vector<std::string>&) const {
        return result;
    }

    void P2PSenderDevice::write() {
        // There might be a remnant (but finished) thread from previous write
        if (m_writingThread.joinable()) {
            m_writingThread.join();
        }

        // start extra thread since write is a slot and must not block
        if (get<std::string>("scenario") == "test") {
            const unsigned int dataSize = get<unsigned int>("dataSize");
            m_writingThread = std::jthread(std::bind(&Self::writing, this, dataSize));
        } else {
            m_writingThread = std::jthread(std::bind(&Self::writingProfile, this));
        }

        updateState(State::ACTIVE);
    }

    void P2PSenderDevice::stop() {
        m_stopWriting = true;
    }

    // For machine "Intel(R) Xeon(R) CPU E5-1650 v4 @ 3.60GHz" 12 cpus (7183.79 bogomips/cpu)
    // MemTotal:       32804800 kB
    //
    // Note that these numbers measure the pipeline shortcut as implemented in
    // af64553 Speed up large array serialization and pipelines processing (between 2.2.3 and 2.)
    // Numbers have uncertainties since they contain some polling/message travel overhead...
    //
    // Data size        |   Speed  MBytes/sec
    //------------------+---------------------
    //  100000          |   220.87
    //  1000000         |   1198.32
    //  10000000        |   819.13
    //  50000000        |   950.02
    //  100000000       |   973.16
    //------------------+---------------------


    void P2PSenderDevice::writing(unsigned int dataSize) {
        m_stopWriting = false;

        try {
            const int nData = get<unsigned int>("nData");
            const unsigned int delayInMs = get<unsigned int>("delay");
            const int noData[] = {}; // Also test an empty NDArray:
            Hash data("data", std::vector<long long>(dataSize), "emptyArray",
                      NDArray(noData, sizeof(noData) / sizeof(noData[0])));
            auto& vec = data.get<std::vector<long long> >("data");

            KARABO_LOG_FRAMEWORK_DEBUG << "P2PSenderDevice::writing : nData = " << nData
                                       << ", delay in ms = " << delayInMs
                                       << ", vector<long long>.size = " << vec.size();
            for (size_t i = 1; i <= vec.size(); ++i) vec[i - 1] = i;

            // Loop all the data to be send
            for (int iData = 0; iData < nData && !m_stopWriting; ++iData) {
                // Fill the data object
                data.set("dataId", iData);
                vec[0] = -iData;

                // Write
                writeChannel("output1", data);

                KARABO_LOG_FRAMEWORK_DEBUG << "Written data # " << iData;
                set("currentDataId", iData);
                if (delayInMs > 0) {
                    std::this_thread::sleep_for(milliseconds(delayInMs));
                }
            }
        } catch (const std::exception& eStd) {
            KARABO_LOG_ERROR << "Stop writing since:\n" << eStd.what();
        } catch (...) {
            KARABO_LOG_ERROR << "Stop writing since unknown exception";
        }
        KARABO_LOG_INFO << "Finished loop sending " << get<unsigned int>("nData") << " items";

        // Done, signal EOS token
        signalEndOfStream("output1");

        updateState(State::NORMAL);
    }

    void P2PSenderDevice::writingProfile() {
        m_stopWriting = false;

        try {
            const int nData = get<unsigned int>("nData");
            const unsigned int delayInMs = get<unsigned int>("delay");
            NDArray ndarr1(Dims(256, 256, 128), karabo::data::Types::INT64);
            for (size_t i = 0; i < 100; ++i) ndarr1.getData<long long>()[i] = 0x0102030405060708;
            NDArray ndarr2(Dims(256, 256, 128), karabo::data::Types::INT64);
            for (size_t i = 0; i < 100; ++i) ndarr2.getData<long long>()[i] = 0x1112131415161718;
            NDArray ndarr3(Dims(256, 256, 128), karabo::data::Types::INT64);
            for (size_t i = 0; i < 100; ++i) ndarr3.getData<long long>()[i] = 0x2122232425262728;
            NDArray ndarr4(Dims(256, 256, 128), karabo::data::Types::INT64);
            for (size_t i = 0; i < 100; ++i) ndarr4.getData<long long>()[i] = 0x3132333435363738;
            Hash data1;
            Hash data2;
            Hash data3;
            Hash data4;
            bool safeNDArray = get<bool>("safeNDArray");
            auto channel = this->getOutputChannel("output2");

            auto microseconds_today = []() -> unsigned long long {
                auto now = system_clock::now();
                return round<microseconds>(now - floor<days>(now)).count();
            };

            // Loop all the data to be send
            for (int iData = 0; iData < nData && !m_stopWriting; ++iData) {
                // Fill the data object - for now only dataId.
                data1.set("array", ndarr1);
                data1.set("inTime", microseconds_today());
                OutputChannel::MetaData meta1("source1", Timestamp());

                data2.set("array", ndarr2);
                data2.set("inTime", microseconds_today());
                OutputChannel::MetaData meta2("source2", Timestamp());

                data3.set("array", ndarr3);
                data3.set("inTime", microseconds_today());
                OutputChannel::MetaData meta3("source3", Timestamp());

                data4.set("array", ndarr4);
                data4.set("inTime", microseconds_today());
                OutputChannel::MetaData meta4("source4", Timestamp());

                // Write four items from different sources
                channel->write(data1, meta1);
                channel->write(data2, meta2);
                channel->write(data3, meta3);
                channel->write(data4, meta4);
                // In our scenario, safeNDArray==true is a bit fake:
                // The data sent survives the update and is re-used (i.e. sent again).
                // If the array data inside the loop would be changed that would lead to data corruption.
                // But we do not do that and are anyway only interested in data throughput.
                // To cure that completely, the NDArray would have to be created (and thus its underlying data
                // allocated) inside the loop.
                // Send all four items in one go
                channel->update(safeNDArray);

                KARABO_LOG_INFO << "Written data # " << iData;
                set("currentDataId", iData);

                std::this_thread::sleep_for(milliseconds(delayInMs));
            }
        } catch (const std::exception& eStd) {
            KARABO_LOG_ERROR << "Stop writing since:\n" << eStd.what();
        } catch (...) {
            KARABO_LOG_ERROR << "Stop writing since unknown exception";
        }

        // Done, signal EOS token
        signalEndOfStream("output2");

        updateState(State::NORMAL);
    }

#undef TEST_VECTOR_SIZE

} // namespace karabo
