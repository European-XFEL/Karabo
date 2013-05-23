/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/xms/SlotElement.hh>

#include "ReconfigurableFsm.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::xms;
using namespace log4cpp;

namespace karabo {
    namespace core {


        void ReconfigurableFsm::expectedParameters(karabo::util::Schema& expected) {

            SLOT_ELEMENT(expected).key("slotEndError")
                    .displayedName("Reset")
                    .description("Resets the device in case of an error")
                    .allowedStates("ErrorState")
                    .commit();
        }


        void ReconfigurableFsm::configure(const karabo::util::Hash& input) {
            SLOT0(slotEndError)
        }


        void ReconfigurableFsm::run() {
            startStateMachine();
            runEventLoop();
        }


        void ReconfigurableFsm::allOkStateOnEntry() {

        }


        void ReconfigurableFsm::allOkStateOnExit() {

        }


        void ReconfigurableFsm::errorStateOnEntry() {

        }


        void ReconfigurableFsm::errorStateOnExit() {

        }


    }
}
