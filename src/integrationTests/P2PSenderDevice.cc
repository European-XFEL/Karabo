/* 
 * File:   P2PSenderDeviceDevice.cc
 * Author: haufs
 * 
 * Created on September 20, 2016, 3:49 PM
 */


#include "P2PSenderDevice.hh"

using namespace std;

USING_KARABO_NAMESPACES;

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, P2PSenderDevice)

    void P2PSenderDevice::expectedParameters(Schema& expected) {

        OVERWRITE_ELEMENT(expected).key("state")
                .setNewOptions(State::NORMAL, State::ACTIVE)
                .setNewDefaultValue(State::NORMAL)
                .commit();

        SLOT_ELEMENT(expected).key("write")
                .displayedName("Write")
                .description("Write some data")
                .allowedStates(State::NORMAL)
                .commit();

        Schema data;
        INT32_ELEMENT(data).key("dataId")
                .readOnly()
                .commit();

        STRING_ELEMENT(data).key("sha1")
                .readOnly()
                .commit();

        STRING_ELEMENT(data).key("flow")
                .readOnly()
                .commit();

        VECTOR_INT64_ELEMENT(data).key("data")
                .readOnly()
                .commit();
        
        NDARRAY_ELEMENT(data).key("array")
                .dtype(Types::DOUBLE)
                .shape("100,200,0")
                .commit();

        OUTPUT_CHANNEL(expected).key("output1")
                .displayedName("Output1")
                .dataSchema(data)
                .commit();

        UINT32_ELEMENT(expected).key("nData")
                .displayedName("Number of data")
                .description("Number of data")
                .assignmentOptional().defaultValue(12)
                .reconfigurable()
                .commit();

        UINT32_ELEMENT(expected).key("delay")
                .displayedName("Delay")
                .description("Delay between writes")
                .assignmentOptional().defaultValue(0u)
                .unit(Unit::SECOND)
                .metricPrefix(MetricPrefix::MILLI)
                .reconfigurable()
                .commit();

        UINT32_ELEMENT(expected).key("currentDataId")
                .displayedName("Current Data ID")
                .description("Monitors the currently processed data token")
                .readOnly()
                .commit();

    }


    P2PSenderDevice::P2PSenderDevice(const Hash& config) : Device<>(config) {
        KARABO_SLOT0(write);
    }


    P2PSenderDevice::~P2PSenderDevice() {
        if (m_writingThread.joinable()) {
            m_writingThread.join();
        }

        KARABO_LOG_DEBUG << "As dead as you can be!";
    }


    void P2PSenderDevice::write() {
        // There might be a remnant (but finished) thread from previous write
        if (m_writingThread.joinable()) {
            m_writingThread.join();
        }

        // start extra thread since write is a slot and must not block
        m_writingThread = boost::thread(boost::bind(&Self::writing, this));

        updateState(State::ACTIVE);
    }


#define TEST_VECTOR_SIZE 1000000

    // For machine "Intel(R) Xeon(R) CPU E5-1620 v2 @ 3.70GHz" 8 cpus (7400.60 bogomips/cpu)
    // MemTotal:       32821756 kB
    //
    // Vector size      |   Speed  MBytes/sec
    //------------------+---------------------
    //  100             |   0.00478766
    //  1000            |   0.0478
    //  10000           |   0.478
    //  100000          |   4.78551
    //  1000000         |   47.8558
    //  10000000        |   478.616
    //  50000000        |   958.46
    //  100000000       |   1198.28
    //------------------+---------------------

    void P2PSenderDevice::writing() {
        try {
            const int nData = get<unsigned int>("nData");
            const unsigned int delayInMs = get<unsigned int>("delay");
            std::vector<long long> vec(TEST_VECTOR_SIZE);
            Hash data;

            KARABO_LOG_FRAMEWORK_DEBUG << "P2PSenderDevice::writing : nData = " << nData
                    << ", delay in ms = " << delayInMs << ", vector<long long>.size = " << vec.size();

            for (size_t i = 1; i <= vec.size(); ++i) vec[i-1] = i;

            // Loop all the data to be send
            for (int iData = 0; iData < nData; ++iData) {

                // Fill the data object - for now only dataId.
                data.set("dataId", iData);

                vec[0] = -iData;
                data.set("data", vec);

                // Write
                writeChannel("output1", data);

                KARABO_LOG_FRAMEWORK_DEBUG << "Written data # " << iData;
                set("currentDataId", iData);
                if (delayInMs > 0) boost::this_thread::sleep(boost::posix_time::milliseconds(delayInMs));
            }
        } catch (const std::exception &eStd) {
            KARABO_LOG_ERROR << "Stop writing since:\n" << eStd.what();
        } catch (...) {
            KARABO_LOG_ERROR << "Stop writing since unknown exception";
        }

        // Done, signal EOS token
        signalEndOfStream("output1");

        updateState(State::NORMAL);
    }

#undef TEST_VECTOR_SIZE

}