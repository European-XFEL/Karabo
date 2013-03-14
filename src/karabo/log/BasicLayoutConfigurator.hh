/*
 * $Id: BasicLayoutConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   BasicLayoutConfigurator.hh
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_LOGCONFIG_BASICLAYOUTCONFIGURATOR_HH
#define	KARABO_LOGCONFIG_BASICLAYOUTCONFIGURATOR_HH

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

        class BasicLayoutConfigurator : public LayoutConfigurator {

        public:

            KARABO_CLASSINFO(BasicLayoutConfigurator, "Basic", "1.0")

            BasicLayoutConfigurator();
            virtual ~BasicLayoutConfigurator();
            log4cpp::Layout* create();

            static void expectedParameters(karabo::util::Schema& expected);
            BasicLayoutConfigurator(const karabo::util::Hash& input);

        };

    }
}

#endif	/* KARABO_LOGCONFIG_BASICLAYOUTCONFIGURATOR_HH */
