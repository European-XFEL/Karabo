/*
 * $Id: PatternLayoutConfigurator.cc 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   PatternLayoutConfigurator.cc
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "PatternLayoutConfigurator.hh"
#include "FileAppenderConfigurator.hh"
#include <log4cpp/PatternLayout.hh>

using namespace std;
using namespace karabo::util;
using log4cpp::Layout;
using log4cpp::PatternLayout;

namespace karabo {
    namespace log {


        KARABO_REGISTER_FOR_CONFIGURATION(LayoutConfigurator, PatternLayoutConfigurator)

        
        void PatternLayoutConfigurator::expectedParameters(Schema& expected) {


            STRING_ELEMENT(expected)
                    .description("Set conversion pattern for the layout. See log4cpp documentation.")
                    .key("pattern")
                    .displayedName("FormatPattern")
                    .assignmentOptional() .defaultValue("%d %c %p %m %n")
                    .commit();
        }


        PatternLayoutConfigurator::PatternLayoutConfigurator(const Hash& input) {
            m_pattern = input.get<string > ("pattern");
        }


        log4cpp::Layout * PatternLayoutConfigurator::create() {
            PatternLayout* layout = new log4cpp::PatternLayout();
            layout->setConversionPattern(m_pattern);
            return layout;
        }
    }
}
