/*
 * $Id: FileAppenderConfigurator.cc 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   FileAppenderConfigurator.cc
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on August 26, 2010, 1:12 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "FileAppenderConfigurator.hh"
#include "krb_log4cpp/FileAppender.hh"
#include "krb_log4cpp/Priority.hh"
#include <karabo/util/Hash.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/SimpleElement.hh>
#include <string>
#include <iostream>

using namespace karabo::util;
using namespace krb_log4cpp;


namespace karabo {
    namespace log {


        KARABO_REGISTER_FOR_CONFIGURATION(AppenderConfigurator, FileAppenderConfigurator)


        void FileAppenderConfigurator::expectedParameters(Schema& expected) {


            PATH_ELEMENT(expected)
                    .description("File name")
                    .key("filename")
                    .displayedName("Filename")
                    .isOutputFile()
                    .assignmentOptional().defaultValue("karabo.log")
                    .commit();


            BOOL_ELEMENT(expected)
                    .description("Append mode")
                    .key("append")
                    .displayedName("Append")
                    .assignmentOptional().defaultValue(true)
                    .commit();


            UINT32_ELEMENT(expected)
                    .description("Access mode")
                    .key("mode")
                    .displayedName("AccessMode")
                    .assignmentOptional().defaultValue((unsigned int) 00644)
                    .commit();

        }


        FileAppenderConfigurator::FileAppenderConfigurator(const Hash& input) : AppenderConfigurator(input) {
            configureFilename(input);
            configureAppendMode(input);
            configureAccessMode(input);
        }


        void FileAppenderConfigurator::configureFilename(const karabo::util::Hash& input) {
            m_fileName = input.get<string> ("filename");
        }


        void FileAppenderConfigurator::configureAppendMode(const karabo::util::Hash& input) {
            m_append = input.get<bool>("append");
        }


        void FileAppenderConfigurator::configureAccessMode(const karabo::util::Hash& input) {
            m_accessMode = input.getAs<mode_t > ("mode");
        }


        krb_log4cpp::Appender* FileAppenderConfigurator::create() {
            return new krb_log4cpp::FileAppender(getName(), getFilename().string(), isAppendMode(), getAccessMode());
        }


        const boost::filesystem::path& FileAppenderConfigurator::getFilename() const {
            return m_fileName;
        }


        bool FileAppenderConfigurator::isAppendMode() const {
            return m_append;
        }


        mode_t FileAppenderConfigurator::getAccessMode() const {
            return m_accessMode;
        }
    }
}
