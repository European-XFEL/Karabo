/*
 * $Id: SimpleLayoutConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   SimpleLayoutConfigurator.hh
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_LOGCONFIG_SIMPLELAYOUTCONFIGURATOR_HH
#define	EXFEL_LOGCONFIG_SIMPLELAYOUTCONFIGURATOR_HH

#include "LayoutConfigurator.hh"
#include <karabo/util/Factory.hh>
/**
 * The main European XFEL namespace
 */
namespace exfel {


  /**
   * Namespace for package log
   */
  namespace log {

    class SimpleLayoutConfigurator : public LayoutConfigurator {
    public:
      EXFEL_CLASSINFO(SimpleLayoutConfigurator, "Simple", "1.0")

      SimpleLayoutConfigurator();
      virtual ~SimpleLayoutConfigurator();

      log4cpp::Layout* create();

      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);

    };

  }
}

#endif	/* EXFEL_LOGCONFIG_SIMPLELAYOUTCONFIGURATOR_HH */
