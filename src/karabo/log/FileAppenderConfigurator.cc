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
#include "log4cpp/FileAppender.hh"
#include "log4cpp/Priority.hh"
#include <karabo/util/Hash.hh>
#include <string>
#include <iostream>

using namespace exfel::util;
using namespace log4cpp;


namespace exfel {
  namespace log {

    FileAppenderConfigurator::FileAppenderConfigurator() {
    }

    FileAppenderConfigurator::~FileAppenderConfigurator() {
    }

    log4cpp::Appender* FileAppenderConfigurator::create() {
      return new log4cpp::FileAppender(getName(), getFilename().string(), isAppendMode(), getAccessMode());
    }

    void FileAppenderConfigurator::expectedParameters(Schema& expected) {


      PATH_ELEMENT(expected)
              .description("File name")
              .key("filename")
              .displayedName("Filename")
              .assignmentOptional()
              .defaultValue(boost::filesystem::path("application.log"))
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

    void FileAppenderConfigurator::configure(const Hash& input) {
      configureFilename(input);
      configureAppendMode(input);
      configureAccessMode(input);
    }

    void FileAppenderConfigurator::configureFilename(const exfel::util::Hash& input) {
      m_fileName = input.get<boost::filesystem::path > ("filename");
    }

    void FileAppenderConfigurator::configureAppendMode(const exfel::util::Hash& input) {
      m_append = input.get<bool>("append");
    }

    void FileAppenderConfigurator::configureAccessMode(const exfel::util::Hash& input) {
      m_accessMode = input.getNumeric<mode_t > ("mode");
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

    EXFEL_REGISTER_FACTORY_CC(AppenderConfigurator, FileAppenderConfigurator)



  }
}
