/*
 * $Id: Shape.hh 5642 2012-03-20 13:11:09Z jszuba $
 *
 * File:   Shape.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 6, 2010, 5:41 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_SHAPE_HH
#define	KARABO_UTIL_SHAPE_HH

#include <string>
#include <sstream>

#include <karabo/util/Factory.hh>
#include "utiltestdll.hh"


namespace karabo {
    namespace util {

        class Shape {

            private:
            std::string m_name;

        public:

            Shape(std::string name) : m_name(name) {
            }

            virtual ~Shape() {
            }

            std::string getName() {
                return m_name;
            }

            virtual std::string draw() = 0;

        };

        class DECLSPEC_UTILTEST ConfigurableShape {

            protected:
            boost::shared_ptr<Shape> m_shape;
            std::string m_name;

        public:

            KARABO_CLASSINFO(ConfigurableShape, "Shape", "1.0")
            KARABO_FACTORY_BASE_CLASS

            ConfigurableShape() {
            }

            virtual ~ConfigurableShape() {
            }

            static void expectedParameters(Schema& expected) {

                STRING_ELEMENT(expected)
                        .key("name")
                        .displayedName("Shape Name")
                        .description("Shape name")
                        .assignmentMandatory()
                        .init()
                        .commit();
            }

            virtual void configure(const Hash & conf) {
                m_name = conf.get<std::string > ("name");
            }

            boost::shared_ptr<Shape> getShape() {
                return m_shape;
            }

            virtual std::string draw() {
                return m_shape->draw();
            }

        };

    }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::util::ConfigurableShape, TEMPLATE_UTILTEST, DECLSPEC_UTILTEST)

#endif	

