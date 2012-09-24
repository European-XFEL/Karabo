/*
 * $Id: SpecializedBobbyCar.hh 4643 2012-08-20 13:24:16Z coppola@DESY.DE $
 *
 * File:   SpecializedBobbyCar.hh
 * Author: <nicola.coppola@xfel.eu>
 *
 * Created on August 20, 2012, 13:25
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_UTIL_SPECIALIZEDBOBBYCAR_HH
#define	EXFEL_UTIL_SPECIALIZEDBOBBYCAR_HH

#include "BobbyCar.hh"
#include "Vehicle.hh"
#include <string>
#include "../Schema.hh"

namespace exfel {

  namespace util {

    class SpecializedBobbyCar : public BobbyCar {
    public:
      EXFEL_CLASSINFO(SpecializedBobbyCar, "SpecializedBobbyCar", "1.0")
              
      template <class Derived>
      SpecializedBobbyCar(Derived* derived) : BobbyCar(derived) {
      }
      
      //SpecializedBobbyCar();

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
