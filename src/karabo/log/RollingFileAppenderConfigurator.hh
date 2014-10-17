/*
 * $Id: RollingFileAppenderConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   RollingFileAppenderConfigurator.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_LOGCONFIG_ROLLINGFILEAPPENDERCONFIGURATOR_HH
#define	KARABO_LOGCONFIG_ROLLINGFILEAPPENDERCONFIGURATOR_HH

#include "FileAppenderConfigurator.hh"
#include <karabo/util/Configurator.hh>
#include <boost/filesystem.hpp>

namespace krb_log4cpp {
    class Appender;
}

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package log
     */
    namespace log {

        class RollingFileAppenderConfigurator : public FileAppenderConfigurator {

            unsigned int m_maxFileSize;
            unsigned short m_maxBackupIndex;

        public:

            KARABO_CLASSINFO(RollingFileAppenderConfigurator, "RollingFile", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            RollingFileAppenderConfigurator(const karabo::util::Hash& input);

            virtual ~RollingFileAppenderConfigurator() {
            };

            krb_log4cpp::Appender* create();

        private: // functions

            void configureMaxSize(const karabo::util::Hash& input);

            void configureMaxBackupIndex(const karabo::util::Hash& input);

        };
    }
}

#endif	/* KARABO_LOGCONFIG_ROLLINGFILEAPPENDERCONFIGURATOR_HH */
