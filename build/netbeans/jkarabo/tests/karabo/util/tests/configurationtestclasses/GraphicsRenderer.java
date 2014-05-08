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
@KARABO_CLASSINFO(classId = "GraphicsRenderer", version = "1.0")
public class GraphicsRenderer extends ClassInfo {

    public static int registration;
    
    static {
        KARABO_REGISTER_FOR_CONFIGURATION(GraphicsRenderer.class, GraphicsRenderer.class);
    }

    public static void expectedParameters(Schema expected) {
        try {
        SLOT_ELEMENT(expected).key("start")
                .displayedName("Start")
                .description("Instructs device to go to started state")
                .allowedStates("Ok.Stopped")
                .commit();

        SLOT_ELEMENT(expected).key("stop")
                .displayedName("Stop")
                .description("Instructs device to go to stopped state")
                .allowedStates("Ok.Started")
                .commit();

        SLOT_ELEMENT(expected).key("reset")
                .displayedName("Reset")
                .description("Resets the device in case of an error")
                .allowedStates("Error")
                .commit();
        
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
                .options("red,green,blue,orange,black")
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
                .description("Some shapes")
                .displayedName("Shapes")
                .appendNodesOfConfigurationBase(Shape.class)
                .assignmentOptional().defaultValue("Rectangle")
                .expertAccess()
                .commit();
        STRING_ELEMENT(expected).key("version")
                .displayedName("Version")
                .description("Version information")
                .readOnly()
                .initialValue("1.4.7")
                .commit();
        } catch (Exception ex) {
            System.out.println("Exception in GraphicsRenderer.expectedParameters: " + ex);
            throw new RuntimeException("Rethrow: " + ex);
        }
    }

    public GraphicsRenderer(Hash input) {
        //cout << input << endl;
        Shape shape = Shape.createChoice(Shape.class, "shapes", input);
        //assert(input.get<string>("version") == "1.4.7");
        //if (input.has("shapes.Circle")) assert(shape->draw() == "Circle");
    }
}
