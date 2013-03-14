/*
 * $Id: LayoutConfigurator.hh 5398 2012-03-07 16:11:30Z wegerk $
 *
 * File:   LayoutConfigurator.hh
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */



#ifndef KARABO_LOGCONFIG_LAYOUTCONFIGURATOR_HH
#define	KARABO_LOGCONFIG_LAYOUTCONFIGURATOR_HH

#include <karabo/util/util.hh>

namespace log4cpp {
    class Layout;
}

namespace karabo {

    namespace util {
        class Hash;
    }

    namespace log {

        class LayoutConfigurator {

        public:

            KARABO_CLASSINFO(LayoutConfigurator, "Layout", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            virtual ~LayoutConfigurator() {
            }
            virtual log4cpp::Layout* create() = 0;

        };
    }
}
// TODO WINDOWS
//KARABO_REGISTER_FACTORY_BASE_HH(karabo::log::LayoutConfigurator, TEMPLATE_LOG, DECLSPEC_LOG)

#endif	/* KARABO_LOGCONFIG_LAYOUTCONFIGURATOR_HH */

