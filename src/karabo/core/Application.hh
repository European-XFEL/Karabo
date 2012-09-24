/*
 * $Id: Application.hh 5399 2012-03-07 16:12:05Z wegerk $
 *
 * File:   Application.hh
 * Author: burkhard.heisen@xfel.eu>
 *
 * Created on September 24, 2010, 2:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_CORE_APPLICATION_HH
#define	EXFEL_CORE_APPLICATION_HH

#include <karabo/util/Factory.hh>
#include "coredll.hh"

#include "Module.hh"


/**
 * The main European XFEL namespace
 */
namespace exfel {

  /**
   * Namespace for package core
   */
  namespace core {

    /**
     * The Application class.
     */
    class Application {
    public:
      EXFEL_CLASSINFO(Application, "Application", "1.0")

      EXFEL_FACTORY_BASE_CLASS
      
      static int runModules(int argc, char** argv);

      Application() {
      };

      static exfel::util::Hash parseCommandLine(int argc, char** argv);

      static void expectedParameters(exfel::util::Schema&);

      void configure(const exfel::util::Hash&);

      virtual ~Application() {
      };

      void run() const;

    protected:



    private:

      std::vector<Module::Pointer> m_modules;

      void loadLogger(const exfel::util::Hash& input);

      void loadModules(const exfel::util::Hash& input);

      static void processOption(const std::string& option, int& argc);

      static void addToConfig(const std::string& key, std::string value, exfel::util::Hash&);

      static void showUsage();

      static void readToken(const std::string& token, exfel::util::Hash& config);
    };



  } // namespace core
} // namespace exfel

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::core::Application, TEMPLATE_CORE, DECLSPEC_CORE)

#endif	/* EXFEL_CORE_APPLICATION_HH */
