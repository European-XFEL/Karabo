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
                .setNewDefaultValue(State::ACTIVE)
                .commit();

        SLOT_ELEMENT(expected).key("write")
                .displayedName("Write")
                .description("Write some data")
                .allowedStates(State::ACTIVE, State::STOPPED)
                .commit();

        INT64_ELEMENT(expected).key("timestamp")
                .displayedName("Timestamp")
                .description("Time point (in seconds since epoch) after write was triggered")
                .readOnly()
                .commit();

        SLOT_ELEMENT(expected).key("stop")
                .displayedName("Stop")
                .description("Stops writing")
                .allowedStates(State::STARTED)
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

    
        FLOAT_ELEMENT(expected).key("payloadSize")
                .displayedName("Payload size")
                .description("Size of the payload for each data token")
                .assignmentOptional().defaultValue(0)
                .reconfigurable()
                .unit(Unit::BYTE)
                .metricPrefix(MetricPrefix::MEGA)
                .commit();

        INT32_ELEMENT(expected).key("nData")
                .displayedName("Number of data")
                .description("Number of data")
                .assignmentOptional().defaultValue(10)
                .reconfigurable()
                .commit();

        BOOL_ELEMENT(expected).key("keepWriting")
                .displayedName("Keep writing")
                .description("Keeps the write button pressed")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .commit();

        INT32_ELEMENT(expected).key("currentDataId")
                .displayedName("Current Data ID")
                .description("Monitors the currently processed data token")
                .readOnly()
                .commit();

        BOOL_ELEMENT(expected).key("randomize")
                .displayedName("Randomize")
                .description("Whether to randomize each data token")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .commit();

     

    }


    P2PSenderDevice::P2PSenderDevice(const Hash& config) : Device<>(config), m_currentDataId(0) {

        // Make the regular c++ functions write and stop callable from outside (i.e. make them SLOTS)
        SLOT0(write);
        SLOT0(stop);

    }

    P2PSenderDevice::~P2PSenderDevice() {
        // There might be a remnant or even running thread from write
        m_isStopped = true;
        if (m_writingThread.joinable()) {
            KARABO_LOG_DEBUG << "Need to join writing thread in destructor!";
            m_writingThread.join();
        }
        
        KARABO_LOG_DEBUG << "As dead as you can be!";
    };

    
    void P2PSenderDevice::write() {
        // There might be a remnant (but finished) thread from previous write
        if (m_writingThread.joinable()) {
            KARABO_LOG_DEBUG << "Old writing thread to join in write()!";
            m_writingThread.join();
        }

        // Log the start time of the whole workflow
        this->set("timestamp", Epochstamp().getSeconds());

        // Adapt own state
        this->updateState(State::STARTED);

        // Set to not stopped
        m_isStopped = false;

        // Initialize random number generator
        srand((unsigned) time(0));

        // start extra thread since write is a slot and must not block
        m_writingThread = boost::thread(boost::bind(&Self::writing, this));
    }
    
    void P2PSenderDevice::writing() {

        // Copy to local variable for performance - stop by calling stop()
        bool keepWriting = this->get<bool>("keepWriting");

        try {
            do { // Loop here, if user wants to keep writing

                // Copy some properties into local members for performance reasons.
                // These are valid for nData data items and user changes can be 
                // recognized for the next bunch (if keepWriting is true)
                const int nData = this->get<int>("nData");
                const bool randomize = this->get<bool>("randomize");
                // array inside do-while to allow changing length after writing:
                vector<long long> array;

                // Loop all the data to be send
                for (int i = 0; i < nData; ++i) {
                
                    //Hash to be sent out
                    Hash::Pointer data;
                
                    // If user pressed stop, we stop any writing
                    if (m_isStopped) {
                        keepWriting = false;
                        break;
                    }

                    // Generate random data
                    if (array.empty() || randomize) {
                        if (array.empty()) {
                            array.resize(this->get<float>("payloadSize") * 1.E6 / sizeof (long long));
                        }
                        for (size_t i = 0; i < array.size(); ++i) array[i] = (rand() % 100) + 1;
                    }                   

                    // Fill the data object
                    data->set("dataId", m_currentDataId);
                    data->set("flow", getInstanceId() + ":output1");
                    data->set("data", array);

                    // Write channel 1
                    this->writeChannel("output1", data);
                    
                    KARABO_LOG_DEBUG << "Writing data # " << m_currentDataId;
                    set("currentDataId", m_currentDataId);

                    // Increment the dataId
                    m_currentDataId++;
                }

                if (m_isStopped) {
                    m_isStopped = false;
                    break;
                }

            } while (keepWriting);
        } catch (const Exception &e) {
            KARABO_LOG_ERROR << "Stop writing since:\n" << e;
        } catch (const std::exception &eStd) {
            KARABO_LOG_ERROR << "Stop writing since:\n" << eStd.what();
        } catch (...) {
            KARABO_LOG_ERROR << "Stop writing since unknown exception";              
        }

        // Done, signal EOS token
        this->signalEndOfStream("output1");

        // Reset dataId counter
        m_currentDataId = 0;

        // Adapt state
        this->updateState(State::STOPPED);
    }


    void P2PSenderDevice::stop() {
        KARABO_LOG_DEBUG << "Stop command received.";
        m_isStopped = true;
    }
}