/*
 * $Id: RollingFileAppenderConfigurator.cc 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   RollingFileAppenderConfigurator.cc
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "RollingFileAppenderConfigurator.hh"
#include <krb_log4cpp/RollingFileAppender.hh>
#include <karabo/util/SimpleElement.hh>

using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace log {


        KARABO_REGISTER_FOR_CONFIGURATION(AppenderConfigurator, FileAppenderConfigurator, RollingFileAppenderConfigurator)


        void RollingFileAppenderConfigurator::expectedParameters(Schema& expected) {

            UINT32_ELEMENT(expected)
                    .description("Maximum file size, default unit MB, allowed range: (0 -1024MB)")
                    .key("maxSize")
                    .displayedName("MaxSize")
                    .minExc(0)
                    .maxInc(1024)
                    .assignmentOptional().defaultValue(10)
                    .commit();

            STRING_ELEMENT(expected)
                    .description("File size unit")
                    .key("maxSizeUnit")
                    .displayedName("maxSizeUnit")
                    .options("B kB MB GB")
                    .assignmentOptional().defaultValue("MB")
                    .commit();

            UINT16_ELEMENT(expected)
                    .description("Maximum backup index")
                    .key("maxBackupIndex")
                    .displayedName("MaxBackupIndex")
                    .minInc(1)
                    .maxInc(256)
                    .assignmentOptional().defaultValue(10)
                    .commit();
        }


        RollingFileAppenderConfigurator::RollingFileAppenderConfigurator(const Hash& input) : FileAppenderConfigurator(input) {
            configureMaxSize(input);
            configureMaxBackupIndex(input);
        }


        void RollingFileAppenderConfigurator::configureMaxSize(const karabo::util::Hash& input) {

            const unsigned int KILO = 1024;
            const unsigned int MEGA = 1024 * KILO;

            map<string, unsigned int> units;
            units["B"] = 1;
            units["kB"] = KILO;
            units["MB"] = MEGA;

            unsigned int unit = MEGA;
            unit = units[input.get<string > ("maxSizeUnit")];
            m_maxFileSize = input.get<unsigned int>("maxSize") * unit;

        }


        void RollingFileAppenderConfigurator::configureMaxBackupIndex(const karabo::util::Hash& input) {
            m_maxBackupIndex = input.get<unsigned short>("maxBackupIndex");
        }


        krb_log4cpp::Appender* RollingFileAppenderConfigurator::create() {

            return new krb_log4cpp::RollingFileAppender(getName(), getFilename().string(),
                                                    m_maxFileSize, m_maxBackupIndex,
                                                    isAppendMode(), getAccessMode());
        }
    }
}
