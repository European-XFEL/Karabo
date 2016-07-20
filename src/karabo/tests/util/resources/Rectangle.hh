/*
 * $Id: Rectangle.hh 4643 2011-11-04 16:04:16Z heisenb@DESY.DE $
 *
 * File:   Rectangle.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on August 6, 2010, 5:41 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_RECTANGLE_HH
#define	KARABO_UTIL_RECTANGLE_HH

#include <string>
#include <sstream>
#include <karabo/util/Factory.hh>
#include "Shape.hh"

namespace karabo {
    namespace util {

/**
         * Rectangle class. Used for test
         */
        class Rectangle : public Shape {


            std::string color;
            int id;
        public:

            Rectangle() : Shape("Rectangle") {
            }

            virtual ~Rectangle() {
            }

            Rectangle(const std::string& color, int id) : Shape("Rectangle") {
                this->color = color;
                this->id = id;
            }

            Rectangle(const std::string& name, const std::string& color, int id) : Shape(name) {
                this->color = color;
                this->id = id;
            }

            std::string draw() {
                std::stringstream ss;
                ss << getName() << " " << id << " " << color;
                return ss.str();
            }

        };

/**
         * This class defines how to configure Rectangle
         * @see Configurable shape
         * Every class which is instantiated by the Factory must implement two methods
         * as seen below. The method configure(Schema&) must be implemented for both based and derived class.
         *
         * This is quite complex example used when there is no access to the original
         * Shape class. Here the bridge pattern is used to avoid multiple inheritance.
         *
         * FACTORY_DERIVED_CLASS_HH(ConfigurableShape, ConfigurableRectangle) is used to define
         * ConfigurableRectangleFactory using typedef
         * This macro must be called outside of the class, but within the same namespace as class.
         */
        class ConfigurableRectangle : public ConfigurableShape {

            public:

            KARABO_CLASSINFO(ConfigurableRectangle, "Rectangle", "1.0")

            ConfigurableRectangle() {
            }

            virtual ~ConfigurableRectangle() {
            }

            /**
             * This method is called by Factory class
             * Get all needed parameters from Hash object and setup the class
             */
            void configure(const Hash& conf) {
                //        Hash rectangle = conf.get<Hash > ("Rectangle");
                //        std::string color = rectangle.get<string > ("color");
                //        int id = rectangle.get<int>("id");
                //        if (rectangle.has("name")) {
                //          m_name = rectangle.get<string > ("name");
                //        }

                std::string color = "red";
                int id = 235;
                m_shape = boost::shared_ptr<Rectangle > (new Rectangle(m_name, color, id));
            }

            /**
             *  This method is called by Factory class
             *  You must define here expected parameters
             *  @param Schema& expected object to be filled
             */
            static void expectedParameters(Schema& expected) {

                FLOAT_ELEMENT(expected)
                        .key("position")
                        .alias(std::vector<int>(4, 1))
                        .displayedName("Position")
                        .description("Position of upper-left corner")
                        .minInc(0.0).maxInc(20.0)
                        .assignmentOptional().defaultValue(0.0)
                        .reconfigurable()
                        .commit();

                FLOAT_ELEMENT(expected)
                        .key("a")
                        .displayedName("a")
                        .description("Horizontal length")
                        .minExc(0.0).maxInc(5.0)
                        .assignmentOptional().defaultValue(1.0)
                        .init()
                        .commit();

                FLOAT_ELEMENT(expected)
                        .key("b")
                        .displayedName("b")
                        .description("Vertical length")
                        .minExc(0.0).maxInc(5.0)
                        .assignmentOptional().defaultValue(1.0)
                        .init()
                        .commit();
            }

        };
    } // namepspace util
} // namespace karabo

#endif	/* KARABO_UTIL_RECTANGLE_HH */

