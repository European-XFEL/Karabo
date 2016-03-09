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

USING_KARABO_NAMESPACES

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, __CLASS_NAME__)

    void __CLASS_NAME__::expectedParameters(Schema& expected) {
    }


    __CLASS_NAME__::__CLASS_NAME__(const karabo::util::Hash& config) : Device<>(config) {
    }


    __CLASS_NAME__::~__CLASS_NAME__() {
    }


    void __CLASS_NAME__::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
    }


    void __CLASS_NAME__::postReconfigure() {
    }
  
}
