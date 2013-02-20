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

#include <karabo/util/karaboDll.hh>

namespace schemaTest {

    struct Shape {
        KARABO_CLASSINFO(Shape, "Shape", "1.0");
        KARABO_CONFIGURATION_BASE_CLASS
        
        virtual std::string draw() const = 0;
    };
    
    //**********************************************
    //                Circle                       *
    //**********************************************

    struct Circle : public Shape {
        KARABO_CLASSINFO(Circle, "Circle", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {
            using namespace karabo::util;

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
        
        std::string draw() const {
            return "circle";
        }
    };
   
    //**********************************************
    //            Editable Circle                  *
    //**********************************************
    
    
    struct EditableCircle : public Circle {
        KARABO_CLASSINFO(EditableCircle, "EditableCircle", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {
            using namespace karabo;

            OVERWRITE_ELEMENT(expected).key("radius")
                    .setNowReconfigurable()
                    .commit();
        }
        
         std::string draw() const {
            return "ecircle";
        }

    };

    //**********************************************
    //                 Rectangle                   *
    //**********************************************
    
    
    struct Rectangle : public Shape {
        KARABO_CLASSINFO(Rectangle, "Rectangle", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {
            using namespace karabo::util;

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
        
         std::string draw() const {
            return "rectangle";
        }
    };
   

    struct GraphicsRenderer {
        KARABO_CLASSINFO(GraphicsRenderer, "GraphicsRenderer", "1.0");
        KARABO_CONFIGURATION_BASE_CLASS
                
        static void expectedParameters(karabo::util::Schema & expected) {
            using namespace karabo::util;

            BOOL_ELEMENT(expected).key("antiAlias")
                    .tag("prop")
                    .displayedName("Use Anti-Aliasing")
                    .description("You may switch of for speed")
                    .assignmentOptional().defaultValue(true)
                    .init()
                    .advanced()
                    .commit();

            STRING_ELEMENT(expected).key("color")
                    .tag("prop")
                    .displayedName("Color")
                    .options("red,green,blue,orange,black")
                    .description("The default color for any shape")
                    .assignmentOptional().defaultValue("red")
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(expected).key("bold")
                    .tag("prop")
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
        
        void configure(const karabo::util::Hash& input) {
            
            std::cout << "*********** " << std::endl << input << std::endl;
            Shape::Pointer shape = Shape::createChoice("shapes", input);
            std::cout << "*********** " <<  shape->draw() << std::endl;
            if (input.has("shapes.Circle")) std::cout << "$$$$$$$$ 0000" << std::endl;
        }
    };
    
    struct GraphicsRenderer1 {
        KARABO_CLASSINFO(GraphicsRenderer1, "GraphicsRenderer1", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {
            using namespace karabo::util;

            BOOL_ELEMENT(expected).key("antiAlias")
                    .tag("prop")
                    .displayedName("Use Anti-Aliasing")
                    .description("You may switch of for speed")
                    .assignmentOptional().defaultValue(true)
                    .init()
                    .advanced()
                    .commit();

            STRING_ELEMENT(expected).key("color")
                    .tag("prop")
                    .displayedName("Color")
                    .description("The default color for any shape")
                    .assignmentOptional().defaultValue("red")            
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(expected).key("bold")
                    .tag("prop")
                    .displayedName("Bold")
                    .description("Toggles bold painting")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();

            CHOICE_ELEMENT(expected).key("shapes")
                    .assignmentOptional().defaultValue("circle")
                    .commit();
                    
                    
            NODE_ELEMENT(expected).key("shapes.circle")
                    .tag("shape")
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
}
#endif
