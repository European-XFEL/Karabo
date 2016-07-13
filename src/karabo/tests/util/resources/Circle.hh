/*
 * $Id: Circle.hh 4643 2011-11-04 16:04:16Z heisenb@DESY.DE $
 *
 * File:   Circle.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 6, 2010, 5:41 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_CIRCLE_HH
#define	KARABO_UTIL_CIRCLE_HH

#include <string>
#include <sstream>
#include <karabo/util/Factory.hh>
#include "Shape.hh"

namespace karabo {
    namespace util {

/**
         * Circle class. Used for test
         */
        class Circle : public Shape {


            std::string color;

        public:

            Circle() : Shape("Circle") {
            }

            Circle(const std::string& color) : Shape("Circle") {
                this->color = color;
            }

            Circle(const std::string& name, const std::string& color) : Shape(name) {
                this->color = color;
            }

            std::string draw() {
                std::stringstream ss;
                ss << getName() << " " << color;
                return ss.str();
            }

        };

/**
         * This class defines how to configure Circle
         * @see Configurable shape
         * Every class which is instantiated by the Factory must implement two methods
         * as seen below. The method configure(Hash&) must be implemented for both based and derived class.
         */
        class ConfigurableCircle : public ConfigurableShape {

            public:

            KARABO_CLASSINFO(ConfigurableCircle, "Circle", "1.0")

            ConfigurableCircle() {
            }

            virtual ~ConfigurableCircle() {
            }

            /**
             * This method is called by Factory class
             * Get all needed parameters from Schema object and setup the class
             */
            void configure(const Hash& conf) {
                //        Hash circle = conf.get<Hash > ("Circle");
                //        std::string color = circle.get<std::string > ("color");
                //        if (circle.has("name")) {
                //          m_name = circle.get<std::string > ("name");
                //        }
                std::string color = "green";
                m_shape = boost::shared_ptr<Circle > (new Circle(m_name, color));
            }

            /**
             *  This method is called by Factory class
             *  You must define here expected parameters
             *  @param Schema& expected object to be filled
             */
            static void expectedParameters(Schema& expected) {

                INT32_ELEMENT(expected)
                        .key("radius")
                        .displayedName("CircleRadius")
                        .description("Circle Radius description")
                        .assignmentOptional().defaultValue(10)
                        .init()
                        .commit();

            }
        };

    } // namepspace util
} // namespace karabo

#endif	/* KARABO_UTIL_CIRCLE_HH */



