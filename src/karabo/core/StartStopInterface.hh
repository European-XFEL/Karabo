/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   StartStopInterface.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 12, 2015, 1:26 PM
 */

#ifndef KARABO_CORE_STARTSTOPINTERFACE_HH
#define KARABO_CORE_STARTSTOPINTERFACE_HH

#include "Device.hh"
#include "karabo/util/OverwriteElement.hh"
#include "karabo/xms/SignalSlotable.hh"
#include "karabo/xms/SlotElement.hh"

namespace karabo {
    namespace core {

        /**
         * @class StartStopInterface
         * @brief suggested interface to work on top of a karabo::core::StartStopFsm
         */
        class StartStopInterface : public virtual karabo::xms::SignalSlotable {
           public:
            KARABO_CLASSINFO(StartStopInterface, "StartStopInterface", "1.3")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;
                using namespace karabo::util;

                OVERWRITE_ELEMENT(expected)
                      .key("state")
                      .setNewOptions(State::INIT, State::ERROR, State::STARTED, State::STOPPING, State::STOPPED,
                                     State::STARTING)
                      .setNewDefaultValue(State::INIT)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("start")
                      .displayedName("Start")
                      .description("Instructs device to go to started state")
                      .allowedStates(State::STOPPED)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("stop")
                      .displayedName("Stop")
                      .description("Instructs device to go to stopped state")
                      .allowedStates(State::STARTED)
                      .commit();


                SLOT_ELEMENT(expected)
                      .key("reset")
                      .displayedName("Reset")
                      .description("Resets the device in case of an error")
                      .allowedStates(State::ERROR)
                      .commit();
            }

            void initFsmSlots() {
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

            void stopFsm() {}
        };
    } // namespace core
} // namespace karabo

#endif
