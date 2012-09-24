/*
 * $Id: PatternLayoutConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   PatternLayoutConfigurator.hh
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_LOGCONFIG_PATTERNLAYOUTCONFIGURATOR_HH
#define	EXFEL_LOGCONFIG_PATTERNLAYOUTCONFIGURATOR_HH

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

    class PatternLayoutConfigurator : public LayoutConfigurator {
    public:
      EXFEL_CLASSINFO(PatternLayoutConfigurator, "Pattern", "1.0")

      PatternLayoutConfigurator();
      virtual ~PatternLayoutConfigurator();
      log4cpp::Layout* create();

      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);

    private:
      std::string m_pattern;

    };

  }
}

#endif	/* EXFEL_LOGCONFIG_PATTERNLAYOUTCONFIGURATOR_HH */
