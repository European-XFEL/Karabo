/*
 * $Id: BasicLayoutConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   BasicLayoutConfigurator.hh
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_LOGCONFIG_BASICLAYOUTCONFIGURATOR_HH
#define	EXFEL_LOGCONFIG_BASICLAYOUTCONFIGURATOR_HH

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

    class BasicLayoutConfigurator : public LayoutConfigurator {
    public:
      
      EXFEL_CLASSINFO(BasicLayoutConfigurator, "Basic", "1.0")

      BasicLayoutConfigurator();
      virtual ~BasicLayoutConfigurator();
      log4cpp::Layout* create();

      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);

    };

  }
}

#endif	/* EXFEL_LOGCONFIG_BASICLAYOUTCONFIGURATOR_HH */
