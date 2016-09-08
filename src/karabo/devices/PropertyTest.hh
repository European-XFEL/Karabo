/* 
 * File:   PropertyTest.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on September 5, 2016, 11:08 AM
 */

#ifndef KARABO_DEVICES_PROPERTYTEST_HH
#define	KARABO_DEVICES_PROPERTYTEST_HH

#include <boost/shared_ptr.hpp>
#include <karabo/karabo.hpp>

namespace karabo {
    namespace devices {
        
        
        struct ShapeProperty {
        
            KARABO_CLASSINFO(ShapeProperty, "ShapeProperty", "1.5")
            KARABO_CONFIGURATION_BASE_CLASS
                    
            ShapeProperty(const karabo::util::Hash& in) : m_configuration(in) {
            }
            
            virtual ~ShapeProperty() {
            }
            
            const karabo::util::Hash & getConfiguration() {
                return m_configuration;
            }
            
            virtual std::string draw() const = 0;
            
        private:
            karabo::util::Hash m_configuration;
        };
        
        
//        struct CircleProperty : public ShapeProperty {
//
//            KARABO_CLASSINFO(CircleProperty, "CircleProperty", "1.5");
//
//            static void expectedParameters(karabo::util::Schema & expected) {
//                using namespace karabo::util;
//                
//                FLOAT_ELEMENT(expected).key("radius").alias(1)
//                        .description("The radius of the circle")
//                        .displayedName("Radius")
//                        .minExc(0)
//                        .maxInc(25)
//                        .unit(Unit::METER)
//                        .metricPrefix(MetricPrefix::MILLI)
//                        .assignmentOptional().defaultValue(10)
//                        .reconfigurable()
//                        .commit();
//            }
//
//            CircleProperty(const karabo::util::Hash & configuration) : ShapeProperty(configuration) {
//            }
//
//            virtual ~CircleProperty() {
//            }
//
//            std::string draw() const {
//                return this->getClassInfo().getClassId();
//            }
//        };

        
//        struct RectangleProperty : public ShapeProperty {
//            
//            KARABO_CLASSINFO(RectangleProperty, "RectangleProperty", "1.5");
//            
//            static void expectedParameters(karabo::util::Schema & expected) {
//                using namespace karabo::util;
//
//                FLOAT_ELEMENT(expected).key("a")
//                        .description("Width")
//                        .displayedName("A")
//                        .minExc(0)
//                        .maxExc(100)
//                        .unit(Unit::METER)
//                        .metricPrefix(MetricPrefix::MILLI)
//                        .assignmentOptional().defaultValue(10)
//                        .adminAccess()
//                        .init()
//                        .commit();
//
//                FLOAT_ELEMENT(expected).key("b")
//                        .description("Height")
//                        .displayedName("B")
//                        .minExc(0)
//                        .maxExc(100)
//                        .unit(Unit::METER)
//                        .metricPrefix(MetricPrefix::MILLI)
//                        .assignmentOptional().defaultValue(10)
//                        .init()
//                        .commit();
//            }
//
//            RectangleProperty(const karabo::util::Hash & configuration) : ShapeProperty(configuration) {
//            }
//
//            virtual ~RectangleProperty() {
//            }
//
//            std::string draw() const {
//                return this->getClassInfo().getClassId();
//            }
//        };
        
        
        class PropertyTest : public karabo::core::Device<> {
        public:
            
            KARABO_CLASSINFO(PropertyTest, "PropertyTest", "1.5")
            
            static void expectedParameters(karabo::util::Schema& expected);
            
            PropertyTest(const karabo::util::Hash& config);
            
            ~PropertyTest();
            
        private:
            
            void initialize();
            
        };
        
    } 
}

#endif	/* KARABO_DEVICES_PROPERTYTEST_HH */

