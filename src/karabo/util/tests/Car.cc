/*
 * $Id: Car.cc 4643 2011-11-04 16:04:16Z heisenb@DESY.DE $
 *
 * File:   Car.cc
 * Author: <your.email@xfel.eu>
 *
 * Created on September 7, 2010, 4:05 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Car.hh"
#include "../Schema.hh"
#include <string>

using namespace std;

namespace exfel {
  namespace util {

    Car::Car() {
    }

    void Car::start() {
      cout << "Starting " << m_name << ", turning " << m_equipment << " on" << endl;
      cout << "By the way, id1 is " << m_idPair.first << " id2 is " << m_idPair.second << endl;
    }

    void Car::stop() {
      cout << "Stopping " << m_name << ", turning " << m_equipment << " off" << endl;
    }

    void Car::expectedParameters(Schema& expected) {

      STRING_ELEMENT(expected)
              .key("name")
              .alias(1)
              .displayedName("Brand")
              .description("Brand of the car")
              .options("Apple,Plum,Cherry")
              .assignmentMandatory()
              .reconfigurable()
              .commit();

      STRING_ELEMENT(expected)
              .key("equipment")
              .alias(2)
              .displayedName("Extra equipment")
              .description("Define extra equipment")
              .options("Radio,AirCondition,Navigation")
              .assignmentOptional().defaultValue("Navigation")
              .reconfigurable()
              .commit();
      
      INTERNAL_ANY_ELEMENT(expected)
              .key("idPair")
              .description("This is to demonstrate the INTERNAL_ANY_ELEMENT")
              .commit();

    }

    void Car::configure(const Hash& conf) {
      conf.get("name", m_name);
      conf.get("equipment", m_equipment);
      if(conf.has("idPair")) {
        conf.get("idPair", m_idPair);
      } else {
        m_idPair = std::pair<int,int>(0,0);
      }
      
    }

    EXFEL_REGISTER_FACTORY_CC(Vehicle, Car)

  } // namespace util
} // namespace exfel
