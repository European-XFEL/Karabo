/*
 * $Id: PatternLayoutConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   PatternLayoutConfigurator.hh
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_LOGCONFIG_PATTERNLAYOUTCONFIGURATOR_HH
#define	KARABO_LOGCONFIG_PATTERNLAYOUTCONFIGURATOR_HH

#include "LayoutConfigurator.hh"
#include <karabo/util/util.hh>
/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package log
     */
    namespace log {

        class PatternLayoutConfigurator : public LayoutConfigurator {

            std::string m_pattern;
            
        public:
            KARABO_CLASSINFO(PatternLayoutConfigurator, "Pattern", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            PatternLayoutConfigurator(const karabo::util::Hash& input);

            virtual ~PatternLayoutConfigurator(){};

            log4cpp::Layout* create();

        };
    }
}

#endif	/* KARABO_LOGCONFIG_PATTERNLAYOUTCONFIGURATOR_HH */
