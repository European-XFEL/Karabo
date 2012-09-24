/*
 * $Id: ParameterCheck.hh 5644 2012-03-20 13:17:40Z jszuba $
 *
 * File:   ParameterCheck.hh
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on December 1, 2010, 4:56 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_IO_PARAMETERCHECK_HH
#define	EXFEL_IO_PARAMETERCHECK_HH

#include <string>
#include <exfel/util/Factory.hh>
#include "iotestdll.hh"


namespace exfel {

  namespace io {

    class ParameterCheck {
    public:
      
      EXFEL_CLASSINFO(ParameterCheck, "ParameterCheck", "1.0")

      EXFEL_FACTORY_BASE_CLASS

      ParameterCheck() {
      }

      virtual ~ParameterCheck() {}
      
      static void expectedParameters(exfel::util::Schema& expected);

      void configure(const exfel::util::Hash& input);

    private:

    };
  } // namespace io
} // namespace exfel

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::ParameterCheck, TEMPLATE_IOTEST, DECLSPEC_IOTEST)

#endif

