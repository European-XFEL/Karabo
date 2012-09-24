/*
 * $Id: Vehicle.hh 5642 2012-03-20 13:11:09Z jszuba $
 *
 * File:   Vehicle.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on September 7, 2010, 3:59 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_UTIL_VEHICLE_HH
#define	EXFEL_UTIL_VEHICLE_HH

#include "../Factory.hh"
#include "utiltestdll.hh"

namespace exfel {
  namespace util {

    class DECLSPEC_UTILTEST Vehicle {
    public:

      EXFEL_CLASSINFO(Vehicle, "Vehicle", "1.0")

      EXFEL_FACTORY_BASE_CLASS

      virtual ~Vehicle(){}

      virtual void start() = 0;
      virtual void stop() = 0;

      static void expectedParameters(Schema& expected) {


        STRING_ELEMENT(expected)
                .key("color")
                .displayedName("Color")
                .description("Per default paint a vehicle like this")
                .options("red,blue,green,orange")
                .assignmentOptional().defaultValue("red")
                .alias(1)
                .init() // was I
                .commit()
                ;
      }

      void configure(const Hash& input) {
        input.get("color", m_color);
      }

      virtual void reconfigure(const Hash& input) {
      };



    protected:

      std::string m_color;

    };

  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::util::Vehicle, TEMPLATE_UTILTEST, DECLSPEC_UTILTEST)

#endif

