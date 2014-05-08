/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests.configurationtestclasses;

import karabo.util.ClassInfo;
import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;

/**
 *
 * @author Sergey Esenov <serguei.essenov at xfel.eu>
 */
@KARABO_CLASSINFO(classId = "Shape", version = "1.0")
public abstract class Shape extends ClassInfo {

    private Hash m_configuration;
    public static int registration;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Shape.class, Shape.class);
    }

    public static void expectedParameters(Schema expected) {
        BOOL_ELEMENT(expected).key("shadowEnabled")
                .description("Shadow enabled")
                .displayedName("Shadow")
                .init()
                .assignmentOptional().defaultValue(false)
                .commit();
    }

    public Shape(Hash configuration) {
        m_configuration = configuration;
    }

    public Hash getConfiguration() {
        return m_configuration;
    }

    public abstract String draw();
}
