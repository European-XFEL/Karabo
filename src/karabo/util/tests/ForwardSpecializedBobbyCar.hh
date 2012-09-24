/*
 * $Id: ForwardSpecializedBobbyCar.hh 4643 2012-08-20 13:24:16Z coppola@DESY.DE $
 *
 * File:   ForwardSpecializedBobbyCar.hh
 * Author: <nicola.coppola@xfel.eu>
 *
 * Created on August 20, 2012, 13:25
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_UTIL_FORWARDSPECIALIZEDBOBBYCAR_HH
#define	EXFEL_UTIL_FORWARDSPECIALIZEDBOBBYCAR_HH

#include "SpecializedBobbyCar.hh"
#include "Vehicle.hh"
#include <string>
#include "../Schema.hh"

namespace exfel {

  namespace util {

    class ForwardSpecializedBobbyCar : public SpecializedBobbyCar {
    public:
      EXFEL_CLASSINFO(ForwardSpecializedBobbyCar, "ForwardSpecializedBobbyCar", "1.0")
              

      ForwardSpecializedBobbyCar();

      void start();
      void stop();

      static void expectedParameters(Schema&);

      void configure(const Hash&);

    private:
      std::string m_name, m_equipment;

    };

  } 
} 

#endif
