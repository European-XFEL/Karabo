/*
 * $Id: LayoutConfigurator.hh 5398 2012-03-07 16:11:30Z wegerk $
 *
 * File:   LayoutConfigurator.hh
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */



#ifndef EXFEL_LOGCONFIG_LAYOUTCONFIGURATOR_HH
#define	EXFEL_LOGCONFIG_LAYOUTCONFIGURATOR_HH

#include <karabo/util/Factory.hh>
#include "logdll.hh"

namespace log4cpp {
  class Layout;
}

namespace exfel {

  namespace util {
    class Hash;
  }

  namespace log {

    class LayoutConfigurator {
    public:

      EXFEL_CLASSINFO(LayoutConfigurator, "Layout", "1.0")
      EXFEL_FACTORY_BASE_CLASS
      virtual ~LayoutConfigurator() {
      }
      virtual log4cpp::Layout* create() = 0;

    };
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::log::LayoutConfigurator, TEMPLATE_LOG, DECLSPEC_LOG)

#endif	/* EXFEL_LOGCONFIG_LAYOUTCONFIGURATOR_HH */

