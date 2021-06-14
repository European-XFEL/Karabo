/*
 * Author: __EMAIL__
 *
 * Created on __DATE__
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "__CLASS_NAME__.hh"

#include "karabo/net/EventLoop.hh"
#include "karabo/util/Exception.hh"
#include "karabo/util/NodeElement.hh"
#include "karabo/util/OverwriteElement.hh"
#include "karabo/util/State.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/TableElement.hh"
#include "karabo/util/VectorElement.hh"
#include "karabo/util/Units.hh"
#include "karabo/xms/SlotElement.hh"


namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(
        karabo::core::BaseDevice, karabo::core::Device<>, __CLASS_NAME__)

    void __CLASS_NAME__::expectedParameters(karabo::util::Schema& expected) {
    }

    __CLASS_NAME__::__CLASS_NAME__ (const karabo::util::Hash& config) :
        karabo::core::Device<>(config) {

            // TODO: Add remaining initialization statements.

            KARABO_INITIAL_FUNCTION(initialize);
    }


    __CLASS_NAME__::~__CLASS_NAME__() {
    }


    void __CLASS_NAME__::preReconfigure(
        karabo::util::Hash& incomingReconfiguration) {
    }


    void __CLASS_NAME__::postReconfigure() {
    }

    void __CLASS_NAME__::initialize() {

        // TODO: add any initialization required after the device has been
        //       created with its starting config (e.g. connect to another
        //       device, start some background task, ...). Potentially lengthy
        //       operations should be performed in here, not in the device's
        //       constructor.

    }
}
