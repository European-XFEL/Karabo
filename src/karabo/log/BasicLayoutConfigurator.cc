/*
 * $Id: BasicLayoutConfigurator.cc 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   BasicLayoutConfigurator.cc
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "BasicLayoutConfigurator.hh"
#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include <log4cpp/BasicLayout.hh>

using namespace std;
using namespace exfel::util;

namespace exfel {
  namespace log {
   

    BasicLayoutConfigurator::BasicLayoutConfigurator() {
    }

    BasicLayoutConfigurator::~BasicLayoutConfigurator() {
    }

    void BasicLayoutConfigurator::expectedParameters(Schema& expected) {
    }

    void BasicLayoutConfigurator::configure(const Hash& input) {
    }

    log4cpp::Layout* BasicLayoutConfigurator::create() {
      return new log4cpp::BasicLayout();
    }

    EXFEL_REGISTER_FACTORY_CC(LayoutConfigurator, BasicLayoutConfigurator)

  }
}
