/*
 * $Id: BobbyCar.cc 5111 2012-02-13 09:35:19Z wrona $
 *
 * File:   BobbyCar.cc
 * Author: <your.email@xfel.eu>
 *
 * Created on September 15, 2010, 12:38 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "BobbyCar.hh"
#include "Shape.hh"
#include "Circle.hh"
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/SingleElement.hh>

using namespace std;

namespace karabo {
    namespace util {


        BobbyCar::BobbyCar() {
        }


        void BobbyCar::start() {
            cout << "Starting " << m_color << " " << m_name << ", turning " << m_equipment << " on" << endl;
        }


        void BobbyCar::stop() {
            cout << "Stopping " << m_color << " " << m_name << ", turning " << m_equipment << " off" << endl;
        }


        void BobbyCar::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected).key("name")
                    .displayedName("Brand")
                    .description("Brand of the BobbyCar")
                    .options("Summer,Winter,Auto")
                    .assignmentMandatory()
                    .reconfigurable()
                    .commit();

            STRING_ELEMENT(expected).key("equipment")
                    .displayedName("Extra equipment")
                    .description("Define extra equipment")
                    .options("Radio,AirCondition,Navigation")
                    .assignmentOptional().defaultValue("Radio")
                    .reconfigurable()
                    .commit();

            CHOICE_ELEMENT<ConfigurableShape > (expected)
                    .key("shape")
                    .displayedName("Car shape")
                    .description("Describe the shape of the car (artificial param)")
                    .assignmentOptional().defaultValue("Circle")
                    .reconfigurable()
                    .commit();

            SINGLE_ELEMENT<ConfigurableShape, ConfigurableCircle > (expected)
                    .key("MyCircle")
                    .description("The circle as SINGLE_ELEMENT")
                    .displayedName("Circle")
                    .assignmentOptional().defaultValue("Circle")
                    .commit();

        }


        void BobbyCar::configure(const karabo::util::Hash& conf) {
            conf.get<std::string > ("name", m_name);
            conf.get("equipment", m_equipment);
        }

        KARABO_REGISTER_FACTORY_CC(Vehicle, BobbyCar)

    }
}
