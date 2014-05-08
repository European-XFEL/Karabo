/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests.configurationtestclasses;

import karabo.util.Hash;
import karabo.util.Schema;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Units;

/**
 *
 * @author Sergey Esenov <serguei.essenov at xfel.eu>
 */
@KARABO_CLASSINFO(classId = "Circle", version = "1.0")
public class Circle extends Shape {

    public static int registration;
    
    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Shape.class, Circle.class);
    }
    
    public static void expectedParameters(Schema expected) {

        FLOAT_ELEMENT(expected).key("radius").alias(1)
                .description("The radius of the circle")
                .displayedName("Radius")
                .init()
                .minExc(0.0F)
                .maxExc(100.0F)
                .unit(Units.Unit.METER)
                .metricPrefix(Units.MetricPrefix.MILLI)
                .assignmentOptional().defaultValue(10)
                .commit();
    }

    public Circle(Hash configuration) {
        super(configuration);
    }

    @Override
    public String draw() {
        return this.getClassId();
    }
}
