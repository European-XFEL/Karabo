/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "CameraFsm.hh"
#include <karabo/xms/SlotElement.hh>

using namespace std;
using namespace karabo::util;
using namespace karabo::xms;
using namespace log4cpp;

namespace karabo {
    namespace core {


        void CameraFsm::expectedParameters(karabo::util::Schema& expected) {

            SLOT_ELEMENT(expected).key("slotEndError")
                    .displayedName("Reset")
                    .description("Resets the camera in case of an error")
                    .allowedStates("Error")
                    .commit();

            SLOT_ELEMENT(expected).key("slotAcquire")
                    .displayedName("Acquire")
                    .description("Instructs camera to go into acquisition state")
                    .allowedStates("AllOk.Ready")
                    .commit();

            SLOT_ELEMENT(expected).key("slotStop")
                    .displayedName("Stop")
                    .description("Instructs camera to stop current acquisition")
                    .allowedStates("AllOk.Acquisition")
                    .commit();

            SLOT_ELEMENT(expected).key("slotTrigger")
                    .displayedName("Trigger")
                    .description("Sends a software trigger to the camera")
                    .allowedStates("AllOk.Acquisition")
                    .commit();
        }


        void CameraFsm::configure(const karabo::util::Hash& input) {
            SLOT0(slotEndError)
            SLOT0(slotAcquire)
            SLOT0(slotStop)
            SLOT0(slotTrigger)
        }


        void CameraFsm::run() {
            startStateMachine();
            runEventLoop();
        }


        void CameraFsm::errorStateOnEntry() {

        }


        void CameraFsm::errorStateOnExit() {

        }


        void CameraFsm::hardwareSetupStateOnEntry() {

        }


        void CameraFsm::hardwareSetupStateOnExit() {

        }


        void CameraFsm::acquisitionStateOnEntry() {

        }


        void CameraFsm::acquisitionStateOnExit() {

        }


        void CameraFsm::readyStateOnEntry() {

        }


        void CameraFsm::readyStateOnExit() {

        }


        void CameraFsm::acquireAction() {

        }


        void CameraFsm::stopAction() {

        }


        void CameraFsm::triggerAction() {

        }
    }
}
