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

        class CameraInterface : public virtual karabo::xms::SignalSlotable {
        public:

            KARABO_CLASSINFO(CameraInterface, "CameraInterface", "1.3")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;
                using namespace karabo::util;

                OVERWRITE_ELEMENT(expected).key("state")
                        .setNewOptions("Initializing,HardwareError,Acquiring,Ready")
                        .setNewDefaultValue("Initializing")
                        .commit();

                SLOT_ELEMENT(expected).key("acquire")
                        .displayedName("Acquire")
                        .description("Instructs camera to go into acquisition state")
                        .allowedStates("Ready")
                        .commit();

                SLOT_ELEMENT(expected).key("trigger")
                        .displayedName("Trigger")
                        .description("Sends a software trigger to the camera")
                        .allowedStates("Acquiring")
                        .commit();

                SLOT_ELEMENT(expected).key("stop")
                        .displayedName("Stop")
                        .description("Instructs camera to stop current acquisition")
                        .allowedStates("Acquiring")
                        .commit();

                SLOT_ELEMENT(expected).key("resetHardware")
                        .displayedName("Reset")
                        .description("Resets the camera in case of an error")
                        .allowedStates("HardwareError")
                        .commit();
                
                Schema data;
                IMAGEDATA(data).key("image")
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
                
                NODE_ELEMENT(expected).key("imageStorage")
                        .displayedName("Local Image Storage")
                        .commit();
                
                BOOL_ELEMENT(expected).key("imageStorage.enable")
                        .displayedName("Enable")
                        .description("Save images while acquiring.")
                        .assignmentOptional().defaultValue(false)
                        .reconfigurable()
                        .allowedStates("Ready")
                        .commit();

                PATH_ELEMENT(expected).key("imageStorage.filePath")
                        .displayedName("File Path")
                        .description("The path for saving images to file")
                        .isDirectory()
                        .assignmentOptional().defaultValue("/tmp")
                        .reconfigurable()
                        .allowedStates("Ready")
                        .commit();

                STRING_ELEMENT(expected).key("imageStorage.fileName")
                        .displayedName("File Name")
                        .description("The name for saving images to file")
                        .assignmentOptional().defaultValue("image")
                        .reconfigurable()
                        .allowedStates("Ready")
                        .commit();

                STRING_ELEMENT(expected).key("imageStorage.fileType")
                        .displayedName("File Type")
                        .description("The image format to be used for writing to file")
                        .assignmentOptional().defaultValue("tif")
                        .options("tif jpg png")
                        .reconfigurable()
                        .allowedStates("Ready")
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
                        .allowedStates("HardwareError,Acquiring,Ready")
                        .commit();

            }

            void initFsmSlots() {
                KARABO_SLOT(acquire);
                KARABO_SLOT(trigger);
                KARABO_SLOT(stop);
                KARABO_SLOT(resetHardware);
            }

            /* Initializing, none, Ready
             * Ready, acquire, Acquiring
             * Acquiring, stop, Ready
             * Acquiring, trigger, None
             * HardwareError, reset, Initializing
             
             * From any state we may be driven to hardwareError
             */
               
            /**
             * In the end call: updateState("Initializing")
             */
            virtual void resetHardware() = 0;
            
          
            /**
             * Should end in "Acquiring"
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
            
            void stopFsm() { }
        };
    } // namespace core
} // namespace karabo

#endif	/* KARABO_CORE_CAMERAINTERFACE_HH */

