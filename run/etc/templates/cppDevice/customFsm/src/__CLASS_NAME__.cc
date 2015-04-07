/*
 * $Id$
 *
 * Author: <__EMAIL__>
 * 
 * Created on __DATE__
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#include "__CLASS_NAME__.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::log;
using namespace karabo::io;
using namespace karabo::net;
using namespace karabo::xms;
using namespace karabo::core;
using namespace karabo::xip;

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, __CLASS_NAME__)

    void __CLASS_NAME__::expectedParameters(Schema& expected) {

        SLOT_ELEMENT(expected).key("inject")
                .displayedName("Inject")
                .description("Injects parameters")
                .allowedStates("Ok.Uninjected")
                .commit();


        SLOT_ELEMENT(expected).key("reset")
                .displayedName("Reset")
                .description("Resets the device in case of an error")
                .allowedStates("Error")
                .commit();

        STRING_ELEMENT(expected).key("result")
                .displayedName("Result")
                .description("The resultant word from the injection")
                .readOnly()
                .commit();

    }


    __CLASS_NAME__::__CLASS_NAME__(const karabo::util::Hash& config) : Device<>(config) {

        // Make the events into the state-machine call-able remotely by making them slots
        SLOT0(inject);
        SLOT0(uninject);
        SLOT0(reset);
        SLOT1(errorFound, std::string /*error message*/);
    }


    __CLASS_NAME__::~__CLASS_NAME__() {
    }


    void __CLASS_NAME__::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
    }


    void __CLASS_NAME__::postReconfigure() {
    }


    void __CLASS_NAME__::errorFoundAction(const std::string& errorMessage) {
        KARABO_LOG_ERROR << errorMessage;
    }


    void __CLASS_NAME__::injectAction() {

        // Create an empty schema
        Schema schema;

        // And fill it with expected parameters        
        SLOT_ELEMENT(schema).key("uninject")
                .displayedName("Uninject")
                .description("Uninjects parameters")
                .allowedStates("Ok.Injected")
                .commit();

        STRING_ELEMENT(schema).key("word")
                .displayedName("Word")
                .description("The word")
                .assignmentOptional().defaultValue("Hello")
                .reconfigurable()
                .commit();

        // Tell the distributed system to update the current schema
        this->updateSchema(schema);
    }


    void __CLASS_NAME__::uninjectAction() {
        set("result", get<string>("word"));

        // Remove the previously injected information by providing and empty Schema
        this->updateSchema(Schema());
    }




}
