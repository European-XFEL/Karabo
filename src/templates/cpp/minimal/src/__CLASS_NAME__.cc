/*
 * Author: __EMAIL__
 *
 * Created on __DATE__
 * from template '__TEMPLATE_ID__' of Karabo __KARABO_VERSION__
 *
 * This file is intended to be used together with Karabo:
 *
 * http://www.karabo.eu
 *
 * IF YOU REQUIRE ANY LICENSING AND COPYRIGHT TERMS, PLEASE ADD THEM HERE.
 * Karabo itself is licensed under the terms of the MPL 2.0 license.
 */

#include "__CLASS_NAME__.hh"

#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/OverwriteElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/TableElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/State.hh"
#include "karabo/data/types/Units.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/xms/SlotElement.hh"


namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device, __CLASS_NAME__)

    void __CLASS_NAME__::expectedParameters(karabo::data::Schema& expected) {
        // Define device schema here, i.e. properties, slots and Input-/OutputChannels
    }

    __CLASS_NAME__::__CLASS_NAME__(const karabo::data::Hash& config) : karabo::core::Device(config) {
        // If the device provides slots (remotely callable methods), add them here.
        // If they should be clickable from the GUI, they have to be added to the schema
        // in expectedParameters in addition.
        // KARABO_SLOT(slotFoo); // void slotFoo() should be a member function

        KARABO_INITIAL_FUNCTION(initialize);
    }


    __CLASS_NAME__::~__CLASS_NAME__() {}


    void __CLASS_NAME__::preReconfigure(karabo::data::Hash& incomingReconfiguration) {}


    void __CLASS_NAME__::postReconfigure() {}

    void __CLASS_NAME__::initialize() {
        // For any InputChannel (e.g. "input") defined in expectedParameters, register
        // a data processing function here. Signature of member function onData should be
        // void onData(const karabo::data::Hash& data, const karabo::xms::InputChannel::MetaData& meta)
        // KARABO_ON_DATA("input", onData);

        // TODO: add any initialization required after the device has been
        //       created with its starting config (e.g. connect to another
        //       device, start some background task, ...). Potentially lengthy
        //       operations should be performed in here, not in the device's
        //       constructor.
    }
} // namespace karabo
