/*
 * $Id: FileAppenderConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   FileAppenderConfigurator.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on August 26, 2010, 1:12 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_LOGCONFIG_FILEAPPENDERCONFIGURATOR_HH
#define	EXFEL_LOGCONFIG_FILEAPPENDERCONFIGURATOR_HH

#include "AppenderConfigurator.hh"
#include <boost/filesystem.hpp>
#include <string>


namespace log4cpp {
  class Appender;
}
/**
 * The main European XFEL namespace
 */
namespace exfel {

  namespace util {
    class Hash;
  }
  /**
   * Namespace for package log
   */
  namespace log {

    /**
     * Configures log4cpp FileAppender
     */
    class FileAppenderConfigurator : public AppenderConfigurator {
    public:
      EXFEL_CLASSINFO(FileAppenderConfigurator, "File", "1.0")

      FileAppenderConfigurator();
      virtual ~FileAppenderConfigurator();

      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);

      const boost::filesystem::path& getFilename() const;
      bool isAppendMode() const;
      mode_t getAccessMode() const;

    protected:
      virtual log4cpp::Appender* create();

      void configureFilename(const exfel::util::Hash& input);
      void configureAppendMode(const exfel::util::Hash& input);
      void configureAccessMode(const exfel::util::Hash& input);

    private:

      boost::filesystem::path m_fileName;
      bool m_append;
      mode_t m_accessMode;
    };

  }
}

#endif	/* EXFEL_LOGCONFIG_FILEAPPENDERCONFIGURATOR_HH */
