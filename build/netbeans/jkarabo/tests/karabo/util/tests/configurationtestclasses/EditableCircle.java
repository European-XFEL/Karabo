/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests.configurationtestclasses;

import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;

/**
 *
 * @author Sergey Esenov <serguei.essenov at xfel.eu>
 */
@KARABO_CLASSINFO(classId = "EditableCircle", version = "1.0")
public class EditableCircle extends Circle {

    public static int registration;
    
    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Shape.class, EditableCircle.class);
    }

    public static void expectedParameters(Schema expected) {
        OVERWRITE_ELEMENT(expected).key("radius")
                .setNowReconfigurable()
                .commit();
    }

    public EditableCircle(Hash configuration) {
        super(configuration);
    }

    @Override
    public String draw() {
        return this.getClassId();
    }
}
