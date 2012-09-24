/*
 * $Id: ForwardSpecializedBobbyCar.cc 5111 2012-08-20 14:00:19Z coppola $
 *
 * File:   ForwardSpecializedBobbyCar.cc
 * Author: <your.email@xfel.eu>
 *
 * Created on August 20, 2012, 14:00 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "ForwardSpecializedBobbyCar.hh"
#include "Shape.hh"
#include "Circle.hh"

using namespace std;

namespace exfel {
  namespace util {

    ForwardSpecializedBobbyCar::ForwardSpecializedBobbyCar() : SpecializedBobbyCar(this) {
    }
    
    void ForwardSpecializedBobbyCar::start() {
      cout << "Starting " << m_color << " " << m_name << ", turning " << m_equipment << " on" << endl;
    }

    void ForwardSpecializedBobbyCar::stop() {
      cout << "Stopping " << m_color << " " << m_name << ", turning " << m_equipment << " off" << endl;
    }

    void ForwardSpecializedBobbyCar::expectedParameters(Schema& expected) {

      STRING_ELEMENT(expected).key("name")
              .displayedName("Brand")
              .description("Brand of the BobbyCar")
              .options("Autumn,Fall,ThisSummer")
              .assignmentMandatory()
              .reconfigurable()
              .commit();

      STRING_ELEMENT(expected).key("equipment")
              .displayedName("Extra equipment")
              .description("Define extra equipment")
              .options("Radio,AirCondition,Navigation")
              .assignmentOptional().defaultValue("Radio")
              .reconfigurable() //was IW
              .commit();

      CHOICE_ELEMENT<ConfigurableShape > (expected)
              .key("shape")
              .displayedName("Car shape")
              .description("Describe the shape of the car (artificial param)")
              .assignmentOptional().defaultValue("Circle")
              .reconfigurable()
              .commit();
      
      SINGLE_ELEMENT<ConfigurableShape, ConfigurableCircle >(expected)
        .key("MyCircle")        
        .description("The circle as SINGLE_ELEMENT")
        .displayedName("Circle")
        .assignmentOptional().defaultValue("Circle")
        .commit();
      
      STRING_ELEMENT(expected).key("runningDirection")
              .displayedName("Running Direction")
              .description("Define Running Direction")
              //.options("Forward")
              .assignmentOptional().defaultValue("Forward")
              .readOnly()
              //.reconfigurable()
              .commit();

    }

    void ForwardSpecializedBobbyCar::configure(const Hash& conf) {
      conf.get<string > ("name", m_name);
      conf.get("equipment", m_equipment);
      //      ConfigurableShape::Pointer shape = ConfigurableShape::create(conf.get<Hash > ("shape"));

    }

    EXFEL_REGISTER_FACTORY_3_CC(Vehicle, BobbyCar, SpecializedBobbyCar, ForwardSpecializedBobbyCar)

  } 
} 
