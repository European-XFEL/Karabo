/*
 * $Id: SimpleLayoutConfigurator.cc 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   SimpleLayoutConfigurator.cc
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "SimpleLayoutConfigurator.hh"
#include <karabo/util/Factory.hh>
#include <log4cpp/SimpleLayout.hh>


using namespace std;

namespace karabo {
  namespace log {

    SimpleLayoutConfigurator::SimpleLayoutConfigurator() { }

    SimpleLayoutConfigurator::~SimpleLayoutConfigurator() { }

    void SimpleLayoutConfigurator::expectedParameters(karabo::util::Schema& expected) {
    }

    void SimpleLayoutConfigurator::configure(const karabo::util::Hash& input) {
    }

    log4cpp::Layout* SimpleLayoutConfigurator::create(){
      return new log4cpp::SimpleLayout();
    }

    KARABO_REGISTER_FACTORY_CC(LayoutConfigurator, SimpleLayoutConfigurator)

  } 
} 
