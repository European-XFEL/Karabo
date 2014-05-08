/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests.configurationtestclasses;

import karabo.util.Configurator;
import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;
import karabo.util.Units;

/**
 *
 * @author Sergey Esenov <serguei.essenov at xfel.eu>
 */
@KARABO_CLASSINFO(classId = "Rectangle", version = "1.0")
public class Rectangle extends Shape {

    public static int registration;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Shape.class, Rectangle.class);
    }

    public static void expectedParameters(Schema expected) {

        FLOAT_ELEMENT(expected).key("a").alias(1)
                .description("Length of a")
                .displayedName("A")
                .init()
                .minExc(0.0F)
                .maxExc(100)
                .unit(Units.Unit.METER)
                .metricPrefix(Units.MetricPrefix.MILLI)
                .assignmentOptional().defaultValue(10)
                .adminAccess()
                .commit();
        FLOAT_ELEMENT(expected).key("b").alias(1)
                .description("Length of b")
                .displayedName("B")
                .init()
                .minExc(0.0F)
                .maxExc(100)
                .unit(Units.Unit.METER)
                .metricPrefix(Units.MetricPrefix.MILLI)
                .assignmentOptional().defaultValue(10)
                .commit();
    }

    public Rectangle(Hash configuration) {
        super(configuration);
    }

    @Override
    public String draw() {
        return this.getClassId();
    }
}
