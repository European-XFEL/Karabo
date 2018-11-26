/* 
 * File:   CameraInterface.hh
 * Author: heisenb
 *
 * Created on May 4, 2015, 10:45 AM
 */

#ifndef KARABO_CORE_CAMERAINTERFACE_HH
#define	KARABO_CORE_CAMERAINTERFACE_HH

#include <karabo/xms/SlotElement.hh>
#include "Device.hh"

namespace karabo {
    namespace core {

        /**
         * @class CameraInterface
         * @brief suggested interface to work on top of a karabo::core::CameraFsm
         */
        class CameraInterface : public virtual karabo::xms::SignalSlotable {

            public:

            KARABO_CLASSINFO(CameraInterface, "CameraInterface", "1.3")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;
                using namespace karabo::util;

                
                OVERWRITE_ELEMENT(expected).key("state")
                        .setNewOptions(State::INIT, State::UNKNOWN, State::ERROR, State::ACQUIRING, State::ON)
                        .setNewDefaultValue(State::INIT)
                        .commit();

                SLOT_ELEMENT(expected).key("connectCamera")
                        .displayedName("Connect")
                        .description("Connects to the hardware")
                        .allowedStates(State::UNKNOWN)
                        .commit();

                SLOT_ELEMENT(expected).key("acquire")
                        .displayedName("Acquire")
                        .description("Instructs camera to go into acquisition state")
                        .allowedStates(State::ON)
                        .commit();

                SLOT_ELEMENT(expected).key("trigger")
                        .displayedName("Trigger")
                        .description("Sends a software trigger to the camera")
                        .allowedStates(State::ACQUIRING)
                        .commit();

                SLOT_ELEMENT(expected).key("stop")
                        .displayedName("Stop")
                        .description("Instructs camera to stop current acquisition")
                        .allowedStates(State::ACQUIRING)
                        .commit();

                SLOT_ELEMENT(expected).key("resetHardware")
                        .displayedName("Reset")
                        .description("Resets the camera in case of an error")
                        .allowedStates(State::ERROR)
                        .commit();

                Schema data;
                NODE_ELEMENT(data).key("data")
                        .displayedName("Data")
                        .setDaqDataType(DaqDataType::TRAIN)
                        .commit();

                IMAGEDATA_ELEMENT(data).key("data.image")
                        .commit();

                OUTPUT_CHANNEL(expected).key("output")
                        .displayedName("Output")
                        .dataSchema(data)
                        .commit();

                DOUBLE_ELEMENT(expected).key("exposureTime")
                        .displayedName("Exposure Time")
                        .description("The requested exposure time in seconds")
                        .unit(Unit::SECOND)
                        .assignmentOptional().defaultValue(1.0)
                        .minInc(0.02)
                        .maxInc(5.0)
                        .reconfigurable()
                        .commit();

                VECTOR_STRING_ELEMENT(expected).key("interfaces")
                        .displayedName("Interfaces")
                        .description("Describes the interfaces of this device")
                        .readOnly()
                        .initialValue({"Camera"})
                        .commit();

                NODE_ELEMENT(expected).key("imageStorage")
                        .displayedName("Local Image Storage")
                        .commit();

                BOOL_ELEMENT(expected).key("imageStorage.enable")
                        .displayedName("Enable")
                        .description("Save images while acquiring.")
                        .assignmentOptional().defaultValue(false)
                        .reconfigurable()
                        .allowedStates(State::ON)
                        .commit();

                PATH_ELEMENT(expected).key("imageStorage.filePath")
                        .displayedName("File Path")
                        .description("The path for saving images to file")
                        .isDirectory()
                        .assignmentOptional().defaultValue("/tmp")
                        .reconfigurable()
                        .allowedStates(State::ON)
                        .commit();

                STRING_ELEMENT(expected).key("imageStorage.fileName")
                        .displayedName("File Name")
                        .description("The name for saving images to file")
                        .assignmentOptional().defaultValue("image")
                        .reconfigurable()
                        .allowedStates(State::ON)
                        .commit();

                STRING_ELEMENT(expected).key("imageStorage.fileType")
                        .displayedName("File Type")
                        .description("The image format to be used for writing to file")
                        .assignmentOptional().defaultValue("tif")
                        .options("tif jpg png")
                        .reconfigurable()
                        .allowedStates(State::ON)
                        .commit();

                STRING_ELEMENT(expected).key("imageStorage.lastSaved")
                        .displayedName("Last Saved")
                        .description("The name of the last saved image")
                        .readOnly()
                        .commit();

                INT32_ELEMENT(expected).key("pollInterval")
                        .displayedName("Poll Interval")
                        .description("The interval with which the camera should be polled")
                        .unit(Unit::SECOND)
                        .minInc(1)
                        .assignmentOptional().defaultValue(10)
                        .reconfigurable()
                        .allowedStates(State::ERROR, State::ON, State::ACQUIRING)
                        .commit();

            }

            void initFsmSlots() {
                KARABO_SLOT(connectCamera);
                KARABO_SLOT(acquire);
                KARABO_SLOT(trigger);
                KARABO_SLOT(stop);
                KARABO_SLOT(resetHardware);
            }
            /* INIT, none, UNKNOWN
             * UNKNOWN, connect, ON
             * ON, acquire, ACQUIRING
             * ACQUIRING, stop, ON
             * ACQUIRING, trigger, None
             * ON or ACQUIRING, errorFound, ERROR
             * ERROR, reset, ON
             * ON or ACQUIRING or ERROR, disconnect, UNKNOWN
             */

            /**
             * In the end call: updateState(State::ON)
             */
            virtual void resetHardware() = 0;

            /**
             * Should end in State::ON
             */
            virtual void connectCamera() = 0;

            /**
             * Should end in State::ACQUIRING
             */
            virtual void acquire() = 0;

            /**
             * ....
             */
            virtual void stop() = 0;

            virtual void trigger() = 0;

            virtual void initialize() = 0;

            void startFsm() {
                this->initialize();
            }

            void stopFsm() {
            }
        };
    } // namespace core
} // namespace karabo

#endif	/* KARABO_CORE_CAMERAINTERFACE_HH */

