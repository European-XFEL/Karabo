/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests.configurationtestclasses;

import karabo.util.ClassInfo;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;
import karabo.util.Units;
import karabo.util.vectors.VectorInteger;

/**
 *
 * @author Sergey Esenov <serguei.essenov at xfel.eu>
 */
@KARABO_CLASSINFO(classId = "TestStruct1", version = "3.2")
public class TestStruct1 extends ClassInfo {

    public static int registration;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(TestStruct1.class, TestStruct1.class);
    }

    public static void expectedParameters(Schema expected) {

        STRING_ELEMENT(expected).key("exampleKey1")
                .reconfigurable()
                .tags("hardware,poll")
                .displayedName("Example key 1")
                .description("Example key 1 description")
                .options("Radio,Air Condition,Navigation", ",")
                .assignmentOptional().defaultValue("Navigation")
                .userAccess()
                .commit();

        INT32_ELEMENT(expected).key("exampleKey2").alias(10)
                .tags("hardware,poll")
                .displayedName("Example key 2")
                .description("Example key 2 description")
                .init()
                .options("5, 25, 10")
                .minInc(5)
                .maxInc(25)
                .unit(Units.Unit.METER)
                .metricPrefix(Units.MetricPrefix.MILLI)
                .assignmentOptional().defaultValue(10)
                .operatorAccess()
                .commit();

        INT32_ELEMENT(expected).key("exampleKey3").alias(5.5)
                .tags("hardware,set")
                .displayedName("Example key 3")
                .description("Example key 3 description")
                .reconfigurable()
                .allowedStates("AllOk.Started, AllOk.Stopped, AllOk.Run.On, NewState")
                .minExc(10)
                .maxExc(20)
                .assignmentMandatory()
                .expertAccess()
                .commit();

        FLOAT_ELEMENT(expected).key("exampleKey4").alias("exampleAlias4")
                .tags("software")
                .displayedName("Example key 4")
                .description("Example key 4 description")
                .options("1.11      -2.22 5.55")
                .assignmentInternal().noDefaultValue()
                .adminAccess()
                .commit();

        VectorInteger vecIntAlias = new VectorInteger();
        vecIntAlias.add(10);
        vecIntAlias.add(20);
        vecIntAlias.add(30);

        INT64_ELEMENT(expected).key("exampleKey5").alias(vecIntAlias)
                .tags("h/w; d.m.y", " ;")
                .displayedName("Example key 5")
                .description("Example key 5 description")
                .readOnly()
                .initialValue(1442244L)
                .warnLow(-10).warnHigh(10)
                .alarmLow(-20).alarmHigh(20)
                .commit();

        INT32_ELEMENT(expected).key("sampleKey")
                .assignmentOptional().defaultValueFromString("10")
                .reconfigurable()
                .commit();

        INT32_ELEMENT(expected).key("sampleKey2")
                .readOnly()
                .commit();

    }
}
