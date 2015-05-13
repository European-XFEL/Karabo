/* 
 * File:   StartStopInterface.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 12, 2015, 1:26 PM
 */

#ifndef KARABO_CORE_STARTSTOPINTERFACE_HH
#define	KARABO_CORE_STARTSTOPINTERFACE_HH

#include <karabo/xms/SlotElement.hh>
#include "Device.hh"

namespace karabo {
    namespace core {

        class StartStopInterface : public virtual karabo::xms::SignalSlotable {
        public:

            KARABO_CLASSINFO(StartStopInterface, "StartStopInterface", "1.3")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;
                using namespace karabo::util;
                
                OVERWRITE_ELEMENT(expected).key("state")
                        .setNewOptions("Initializing,Error,Started,Stopping,Stopped,Starting")
                        .setNewDefaultValue("Initializing")
                        .commit();
                
                SLOT_ELEMENT(expected).key("start")
                        .displayedName("Start")
                        .description("Instructs device to go to started state")
                        .allowedStates("Stopped")
                        .commit();

                SLOT_ELEMENT(expected).key("stop")
                        .displayedName("Stop")
                        .description("Instructs device to go to stopped state")
                        .allowedStates("Started")
                        .commit();


                SLOT_ELEMENT(expected).key("reset")
                        .displayedName("Reset")
                        .description("Resets the device in case of an error")
                        .allowedStates("Error")
                        .commit();

            }

            void initFsmSlots() {
                setNumberOfThreads(2);
                KARABO_SLOT(start);
                KARABO_SLOT(stop);
                KARABO_SLOT(reset);
            }
            
            // Target state: "Stopped"
            virtual void initialize() = 0;

            // Target state: "Started"
            // You may use: "Starting" if start takes time
            virtual void start() = 0;
            
            // Target state: "Stopped"
            virtual void stop() = 0;
            
            // Target state: "Stopped"
            virtual void reset() = 0;            
            
            void startFsm() {
                this->initialize();
            }
            
            void stopFsm() { }
        };
    } // namespace core
} // namespace karabo

#endif

