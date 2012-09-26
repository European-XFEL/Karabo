/*
 * $Id: SimpleLayoutConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   SimpleLayoutConfigurator.hh
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_LOGCONFIG_SIMPLELAYOUTCONFIGURATOR_HH
#define	KARABO_LOGCONFIG_SIMPLELAYOUTCONFIGURATOR_HH

#include "LayoutConfigurator.hh"
#include <karabo/util/Factory.hh>
/**
 * The main European XFEL namespace
 */
namespace karabo {


  /**
   * Namespace for package log
   */
  namespace log {

    class SimpleLayoutConfigurator : public LayoutConfigurator {
    public:
      KARABO_CLASSINFO(SimpleLayoutConfigurator, "Simple", "1.0")

      SimpleLayoutConfigurator();
      virtual ~SimpleLayoutConfigurator();

      log4cpp::Layout* create();

      static void expectedParameters(karabo::util::Schema& expected);
      void configure(const karabo::util::Hash& input);

    };

  }
}

#endif	/* KARABO_LOGCONFIG_SIMPLELAYOUTCONFIGURATOR_HH */
