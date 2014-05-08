/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests.configurationtestclasses;

import karabo.util.ClassInfo;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;
import karabo.util.vectors.VectorDouble;
import karabo.util.vectors.VectorInteger;

/**
 *
 * @author Sergey Esenov <serguei.essenov at xfel.eu>
 */
@KARABO_CLASSINFO(classId = "OtherSchemaElements", version = "1.0")
public class OtherSchemaElements extends ClassInfo {

    public static int registration;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(OtherSchemaElements.class, OtherSchemaElements.class);
    }

    public static void expectedParameters(Schema expected) {
//        SLOT_ELEMENT(expected).key("slotTest")
//                .displayedName("Reset")
//                .description("Test slot element")
//                .allowedStates("Started, Stopped, Reset")
//                .commit();

        PATH_ELEMENT(expected)
                .description("File name")
                .key("filename")
                .alias(5)
                .displayedName("Filename")
                .reconfigurable()
                .isOutputFile()
                .options("file1, file2")
                .assignmentOptional().defaultValue("karabo.log")
                .commit();
        PATH_ELEMENT(expected)
                .key("testfile")
                .isInputFile()
                .readOnly().initialValue("initFile")
                .alarmHigh("a").alarmLow("b")
                .warnHigh("c").warnLow("d")
                .archivePolicy(Schema.ArchivePolicy.EVERY_10MIN)
                .commit();

        VectorInteger vecInit = new VectorInteger();
        vecInit.add(10);
        vecInit.add(20);
        vecInit.add(30);

        VectorInteger vecWarnL = new VectorInteger();
        vecWarnL.add(3);
        vecWarnL.add(50);
        VectorInteger vecWarnH = new VectorInteger();
        vecWarnH.add(3);
        vecWarnH.add(100);

        VECTOR_INT32_ELEMENT(expected)
                .key("vecInt")
                .readOnly()
                .initialValue(vecInit)
                .warnLow(vecWarnL)
                .warnHigh(vecWarnH)
                .archivePolicy(Schema.ArchivePolicy.EVERY_EVENT)
                .commit();

        VectorDouble vecAlarmL = new VectorDouble();
        vecAlarmL.add(3.);
        vecAlarmL.add(-5.5);
        VectorDouble vecAlarmH = new VectorDouble();
        vecAlarmH.add(3.);
        vecAlarmH.add(7.7);

        VECTOR_DOUBLE_ELEMENT(expected)
                .key("vecDouble")
                .readOnly()
                .alarmLow(vecAlarmL)
                .alarmHigh(vecAlarmH)
                .archivePolicy(Schema.ArchivePolicy.NO_ARCHIVING)
                .commit();
        VECTOR_INT32_ELEMENT(expected)
                .key("vecIntReconfig")
                .reconfigurable()
                .assignmentOptional().defaultValue(vecInit)
                .commit();
        VECTOR_INT32_ELEMENT(expected)
                .key("vecIntReconfigStr")
                .reconfigurable()
                .assignmentOptional().defaultValueFromString("11, 22, 33")
                .commit();
        VECTOR_DOUBLE_ELEMENT(expected)
                .key("vecDoubleReconfigStr")
                .reconfigurable()
                .assignmentOptional().defaultValueFromString("1.1, 2.2, 3.3")
                .commit();
        VECTOR_BOOL_ELEMENT(expected)
                .key("vecBool")
                .tags("h/w; d.m.y", ";")
                .allowedStates("AllOk.Started, AllOk.Stopped")
                .minSize(2)
                .maxSize(7)
                .assignmentMandatory()
                .commit();
//            IMAGE_ELEMENT(expected)
//                    .key("image")
//                    .commit();

    }
}
