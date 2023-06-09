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
 * File:   CameraInterface.hh
 * Author: heisenb
 *
 * Created on May 4, 2015, 10:45 AM
 */

#ifndef KARABO_CORE_CAMERAINTERFACE_HH
#define KARABO_CORE_CAMERAINTERFACE_HH

#include "Device.hh"
#include "karabo/util/NodeElement.hh"
#include "karabo/util/OverwriteElement.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/State.hh"
#include "karabo/util/VectorElement.hh"
#include "karabo/xms/ImageData.hh"
#include "karabo/xms/OutputChannel.hh"
#include "karabo/xms/SignalSlotable.hh"
#include "karabo/xms/SlotElement.hh"

namespace karabo {
    namespace core {

        /**
         * @class CameraInterface
         * @brief suggested interface to work on top of a karabo::core::CameraFsm
         */
        class CameraInterface : public virtual karabo::xms::SignalSlotable {
           public:
            KARABO_CLASSINFO(CameraInterface, "CameraInterface", "1.4")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;
                using namespace karabo::util;


                OVERWRITE_ELEMENT(expected)
                      .key("state")
                      .setNewOptions(State::INIT, State::UNKNOWN, State::ERROR, State::ACQUIRING, State::ON)
                      .setNewDefaultValue(State::INIT)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("connectCamera")
                      .displayedName("Connect")
                      .description("Connects to the hardware")
                      .allowedStates(State::UNKNOWN)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("acquire")
                      .displayedName("Acquire")
                      .description("Instructs camera to go into acquisition state")
                      .allowedStates(State::ON)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("trigger")
                      .displayedName("Trigger")
                      .description("Sends a software trigger to the camera")
                      .allowedStates(State::ACQUIRING)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("stop")
                      .displayedName("Stop")
                      .description("Instructs camera to stop current acquisition")
                      .allowedStates(State::ACQUIRING)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("resetHardware")
                      .displayedName("Reset")
                      .description("Resets the camera in case of an error")
                      .allowedStates(State::ERROR)
                      .commit();

                Schema data;
                NODE_ELEMENT(data).key("data").displayedName("Data").setDaqDataType(DaqDataType::TRAIN).commit();

                IMAGEDATA_ELEMENT(data).key("data.image").commit();

                OUTPUT_CHANNEL(expected).key("output").displayedName("Output").dataSchema(data).commit();

                DOUBLE_ELEMENT(expected)
                      .key("exposureTime")
                      .displayedName("Exposure Time")
                      .description("The requested exposure time in seconds")
                      .unit(Unit::SECOND)
                      .assignmentOptional()
                      .defaultValue(1.0)
                      .minInc(0.02)
                      .maxInc(5.0)
                      .reconfigurable()
                      .commit();

                VECTOR_STRING_ELEMENT(expected)
                      .key("interfaces")
                      .displayedName("Interfaces")
                      .description("Describes the interfaces of this device")
                      .readOnly()
                      .initialValue({"Camera"})
                      .commit();

                INT32_ELEMENT(expected)
                      .key("pollInterval")
                      .displayedName("Poll Interval")
                      .description("The interval with which the camera should be polled")
                      .unit(Unit::SECOND)
                      .minInc(1)
                      .assignmentOptional()
                      .defaultValue(10)
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

            void stopFsm() {}
        };
    } // namespace core
} // namespace karabo

#endif /* KARABO_CORE_CAMERAINTERFACE_HH */
