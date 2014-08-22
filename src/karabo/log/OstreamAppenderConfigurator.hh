/*
 * $Id: OstreamAppenderConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   OstreamAppenderConfigurator.hh
 * Author: <your.email@xfel.eu>
 *
 * Created on August 26, 2010, 1:12 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_LOGCONFIG_OSTREAMAPPENDERCONFIGURATOR_HH
#define	KARABO_LOGCONFIG_OSTREAMAPPENDERCONFIGURATOR_HH

#include "AppenderConfigurator.hh"
#include <karabo/util/Configurator.hh>
#include <krb_log4cpp/Appender.hh>
#include <string>
/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package log
     */
    namespace log {

        class OstreamAppenderConfigurator : public AppenderConfigurator {

            std::string m_out;

        public:

            KARABO_CLASSINFO(OstreamAppenderConfigurator, "Ostream", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            OstreamAppenderConfigurator(const karabo::util::Hash& input);

            virtual ~OstreamAppenderConfigurator() {
            };

            krb_log4cpp::Appender* create();

        };

    }
}

#endif	/* KARABO_LOGCONFIG_OSTREAMAPPENDERCONFIGURATOR_HH */
