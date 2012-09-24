/*
 * $Id: Module.hh 5399 2012-03-07 16:12:05Z wegerk $
 *
 * File:   Module.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 6, 2010, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_CORE_MODULE_HH
#define	EXFEL_CORE_MODULE_HH

#include <karabo/util/Factory.hh>
#include <karabo/io/Reader.hh>
#include "coredll.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

  namespace core {

    /**
     * The Module class.
     */
    class Module {
    public:

      EXFEL_CLASSINFO(Module, "Module", "1.0")
      EXFEL_FACTORY_BASE_CLASS
      
      Module() {
      }

      static void expectedParameters(exfel::util::Schema& expected) {
      }

      virtual ~Module() {
      }

      virtual void compute() = 0;
      const std::string getName() const;

    protected:

    private:
      
    };


  } // namespace core
} // namespace exfel

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::core::Module, TEMPLATE_CORE, DECLSPEC_CORE)

#endif
