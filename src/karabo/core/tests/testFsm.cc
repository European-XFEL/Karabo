/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on December 22, 2011, 09:19 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <krb_log4cpp/Category.hh>

#include <exfel/util/Factory.hh>
#include <exfel/util/Test.hh>
#include <exfel/log/Logger.hh>

#include "StateMachineTest.hh"

using namespace std;
using namespace exfel::util;

int testFsm(int argc, char** argv) {

    try {

        Test t;
        TEST_INIT(t, argc, argv);
        cout << t << endl;

        exfel::log::Logger::Pointer log = exfel::log::Logger::create("Logger", Hash("priority", "DEBUG"));
        log->initialize();
        
        exfel::core::StateMachineTest fsmTest;
        fsmTest.startStateMachine();
        //fsmTest.goToB();
        //fsmTest.goToA(1);
        fsmTest.goToA1();
        //fsmTest.onException("bla", "blub");

    } catch (const Exception& e) {
        cout << e;
        RETHROW;
    }

    return 0;
}



