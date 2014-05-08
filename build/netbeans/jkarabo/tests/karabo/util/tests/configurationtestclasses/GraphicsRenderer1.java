/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.tests.configurationtestclasses;

import karabo.util.ClassInfo;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;

/**
 *
 * @author Sergey Esenov <serguei.essenov at xfel.eu>
 */
@KARABO_CLASSINFO(classId = "GraphicsRenderer1", version = "1.0")
public class GraphicsRenderer1 extends ClassInfo {

    public static int registration;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(GraphicsRenderer1.class, GraphicsRenderer1.class);
    }

    public static void expectedParameters(Schema expected) {

        BOOL_ELEMENT(expected).key("antiAlias")
                .tags("prop")
                .displayedName("Use Anti-Aliasing")
                .description("You may switch of for speed")
                .init()
                .assignmentOptional().defaultValue(true)
                .expertAccess()
                .commit();
        STRING_ELEMENT(expected).key("color")
                .tags("prop")
                .displayedName("Color")
                .description("The default color for any shape")
                .reconfigurable()
                .assignmentOptional().defaultValue("red")
                .commit();
        BOOL_ELEMENT(expected).key("bold")
                .tags("prop")
                .displayedName("Bold")
                .description("Toggles bold painting")
                .reconfigurable()
                .assignmentOptional().defaultValue(false)
                .commit();
        CHOICE_ELEMENT(expected).key("shapes")
                .assignmentOptional().defaultValue("circle")
                .commit();
        NODE_ELEMENT(expected).key("shapes.circle")
                .tags("shape")
                .displayedName("Circle")
                .description("A circle")
                .appendParametersOf(Circle.class)
                .commit();
        NODE_ELEMENT(expected).key("shapes.rectangle")
                .tags("shape")
                .displayedName("Rectangle")
                .description("A rectangle")
                .commit();
        FLOAT_ELEMENT(expected).key("shapes.rectangle.b")
                .description("Rectangle side - b")
                .displayedName("Side B")
                .init()
                .assignmentOptional().defaultValue(10.0F)
                .commit();
        FLOAT_ELEMENT(expected).key("shapes.rectangle.c")
                .description("Rectangle side - c")
                .displayedName("Side C")
                .assignmentOptional().defaultValue(10.F)
                .commit();
        NODE_ELEMENT(expected).key("triangle")
                .displayedName("triangle")
                .description("A triangle (Node element containing no other elements)")
                .commit();
    }
}
