/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_UTIL_MOTOR_HH
#define	EXFEL_UTIL_MOTOR_HH

#include <string>
#include "../Factory.hh"
#include "Vehicle.hh"


namespace exfel {
  namespace util {

    class Motor : public Vehicle {
    public:

      EXFEL_CLASSINFO(Motor, "Motor", "1.0")

      static void expectedParameters(Schema& expected);

      void configure(const Hash& conf);

      void reconfigure(const Hash& conf);

      void start();

      void stop() {
      }
    private:
      float m_position;
      float m_velocity;
      float m_dialPosition;
      float m_offset;

      Hash m_initialParams;

      Schema m_reconfigurationMaster;
      Hash m_reconfigurationParams;

      Schema m_monitorMaster;
      Hash m_monitorParams;

    };

  }
}


#endif	/* EXFEL_UTIL_MOTOR_HH */

