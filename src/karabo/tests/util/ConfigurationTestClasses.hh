/* 
 * Author: <burkhard.heisen>
 *
 * Created on February 5, 2013, 11:06 AM
 */

#ifndef KARABO_UTIL_TEST_EXPECTEDPARAMETERCLASSES_HH
#define	KARABO_UTIL_TEST_EXPECTEDPARAMETERCLASSES_HH

#include <karabo/util/SimpleElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/OverwriteElement.hh>
#include <karabo/util/NodeElement.hh>

#include <karabo/util/Configurator.hh>

#include <karabo/util/karaboDll.hh>

namespace configurationTest {

    using namespace karabo::util;

    struct Shape {
        KARABO_CLASSINFO(Shape, "Shape", "1.0");

        KARABO_CONFIGURATION_BASE_CLASS

                static void expectedParameters(karabo::util::Schema & expected) {

            BOOL_ELEMENT(expected).key("shadowEnabled")
                    .description("Shadow enabled")
                    .displayedName("Shadow")
                    .assignmentOptional().defaultValue(false)
                    .init()
                    .commit();
        }

        Shape(const Hash & configuration) : m_configuration(configuration) {
        }

        const Hash & getConfiguration() {
            return m_configuration;
        }

        virtual std::string draw() const = 0;

        Hash m_configuration;
    };

    //**********************************************
    //                Circle                       *
    //**********************************************

    struct Circle : public Shape {
        KARABO_CLASSINFO(Circle, "Circle", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {

            FLOAT_ELEMENT(expected).key("radius").alias(1)
                    .description("The radius of the circle")
                    .displayedName("Radius")
                    .minExc(0)
                    .maxExc(100)
                    .unit(Units::METER)
                    .metricPrefix(Units::MILLI)
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();
        }

        Circle(const karabo::util::Hash & configuration) : Shape(configuration) {
        }

        std::string draw() const {
            return this->getClassInfo().getClassId();
        }
    };

    //**********************************************
    //            Editable Circle                  *
    //**********************************************

    struct EditableCircle : public Circle {
        KARABO_CLASSINFO(EditableCircle, "EditableCircle", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {
            OVERWRITE_ELEMENT(expected).key("radius")
                    .setNowReconfigurable()
                    .commit();
        }

        EditableCircle(const karabo::util::Hash & configuration) : Circle(configuration) {

        }

        std::string draw() const {
            return this->getClassInfo().getClassId();
        }

    };

    //**********************************************
    //                 Rectangle                   *
    //**********************************************

    struct Rectangle : public Shape {
        KARABO_CLASSINFO(Rectangle, "Rectangle", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {

            FLOAT_ELEMENT(expected).key("a").alias(1)
                    .description("Length of a")
                    .displayedName("A")
                    .minExc(0)
                    .maxExc(100)
                    .unit(Units::METER)
                    .metricPrefix(Units::MILLI)
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();

            FLOAT_ELEMENT(expected).key("b").alias(1)
                    .description("Length of b")
                    .displayedName("B")
                    .minExc(0)
                    .maxExc(100)
                    .unit(Units::METER)
                    .metricPrefix(Units::MILLI)
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();
        }

        Rectangle(const karabo::util::Hash & configuration) : Shape(configuration) {
        }

        std::string draw() const {
            return this->getClassInfo().getClassId();
        }
    };

    struct GraphicsRenderer {
        KARABO_CLASSINFO(GraphicsRenderer, "GraphicsRenderer", "1.0");

        KARABO_CONFIGURATION_BASE_CLASS

                static void expectedParameters(karabo::util::Schema & expected) {

            BOOL_ELEMENT(expected).key("antiAlias")
                    .tags("prop")
                    .displayedName("Use Anti-Aliasing")
                    .description("You may switch of for speed")
                    .assignmentOptional().defaultValue(true)
                    .init()
                    .advanced()
                    .commit();

            STRING_ELEMENT(expected).key("color")
                    .tags("prop")
                    .displayedName("Color")
                    .options("red,green,blue,orange,black")
                    .description("The default color for any shape")
                    .assignmentOptional().defaultValue("red")
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(expected).key("bold")
                    .tags("prop")
                    .displayedName("Bold")
                    .description("Toggles bold painting")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();

            CHOICE_ELEMENT(expected).key("shapes")
                    .description("Some shapes")
                    .displayedName("Shapes")
                    .appendNodesOfConfigurationBase<Shape > ()
                    .assignmentOptional().defaultValue("Rectangle")
                    .commit();
        }

        GraphicsRenderer(const karabo::util::Hash & input) {

            std::cout << "*********** " << std::endl << input << std::endl;
            Shape::Pointer shape = Shape::createChoice("shapes", input);
            std::cout << "*********** " << shape->draw() << std::endl;
            if (input.has("shapes.Circle")) std::cout << "$$$$$$$$ 0000" << std::endl;
        }
    };

    struct GraphicsRenderer1 {
        KARABO_CLASSINFO(GraphicsRenderer1, "GraphicsRenderer1", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {

            BOOL_ELEMENT(expected).key("antiAlias")
                    .tags("prop")
                    .displayedName("Use Anti-Aliasing")
                    .description("You may switch of for speed")
                    .assignmentOptional().defaultValue(true)
                    .init()
                    .advanced()
                    .commit();

            STRING_ELEMENT(expected).key("color")
                    .tags("prop")
                    .displayedName("Color")
                    .description("The default color for any shape")
                    .assignmentOptional().defaultValue("red")
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(expected).key("bold")
                    .tags("prop")
                    .displayedName("Bold")
                    .description("Toggles bold painting")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();

            CHOICE_ELEMENT(expected).key("shapes")
                    .assignmentOptional().defaultValue("circle")
                    .commit();


            NODE_ELEMENT(expected).key("shapes.circle")
                    .tags("shape")
                    .displayedName("Circle")
                    .description("A circle")
                    //.appendParametersOf<Circle > ()
                    .commit();


            FLOAT_ELEMENT(expected).key("shapes.circle.radius").alias(1)
                    .description("The radius of the circle")
                    .displayedName("Radius")
                    .minExc(0)
                    .maxExc(100)
                    .unit(Units::METER)
                    .metricPrefix(Units::MILLI)
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();
        }
    };

        struct TestStruct1 {
        KARABO_CLASSINFO(TestStruct1, "TestStruct1", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {
            using namespace karabo::util;
            STRING_ELEMENT(expected).key("exampleKey1")
                    .tags("h/w")
                    .displayedName("Example key 1")
                    .description("Example key 1 description")
                    .assignmentOptional().defaultValue("Some default string")
                    .reconfigurable()
                    .commit();

            INT32_ELEMENT(expected).key("exampleKey2").alias(10)
                    .tags("h/w")
                    .displayedName("Example key 2")
                    .description("Example key 2 description")
                    .assignmentOptional().defaultValue(-10)
                    .init()
                    .commit();

            UINT32_ELEMENT(expected).key("exampleKey3").alias(5.5)
                    .tags("h/w")
                    .displayedName("Example key 3")
                    .description("Example key 3 description")
                    .assignmentMandatory()
                    .reconfigurable()
                    .commit();

            FLOAT_ELEMENT(expected).key("exampleKey4").alias("exampleAlias4")
                    .tags("hardware")
                    .displayedName("Example key 4")
                    .description("Example key 4 description")
                    .assignmentInternal().noDefaultValue()
                    .commit();

            INT64_ELEMENT(expected).key("exampleKey5").alias("exampleAlias5")
                    .tags("hardware")
                    .displayedName("Example key 5")
                    .description("Example key 5 description")
                    .readOnly()
                    .initialValue(1442244)
                    .commit();
        }
    };

}
#endif
