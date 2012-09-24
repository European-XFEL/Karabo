/*
 * $Id: Car.hh 4643 2011-11-04 16:04:16Z heisenb@DESY.DE $
 *
 * File:   Car.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 7, 2010, 4:05 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_UTIL_CAR_HH
#define	EXFEL_UTIL_CAR_HH

#include "Vehicle.hh"
#include <string>
#include "../Schema.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

  /**
   * Namespace for package util
   */
  namespace util {

    class Car : public Vehicle {
    public:
      
      EXFEL_CLASSINFO(Car, "Car", "1.0")

      Car();

      void start();
      void stop();

      static void expectedParameters(Schema&);

      void configure(const Hash&);

    private:
      std::string m_equipment;
      std::string m_name;
      std::pair<int, int> m_idPair;
    };
  }
}

#endif	/* EXFEL_UTIL_CAR_HH */
