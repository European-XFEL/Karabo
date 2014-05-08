/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.io;

import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
@KARABO_CLASSINFO(classId = "Output", version = "1.0")
public abstract class Output<T extends Object>  extends AbstractOutput {

    protected boolean m_appendModeEnabled = false;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Output.class, Output.class);
        //System.out.println("Output class static registration");
    }

    public static void expectedParameters(Schema expected) {

        BOOL_ELEMENT(expected).key("enableAppendMode")
                .displayedName("Enable append mode")
                .description("NOTE: Has no effect on Output-Network. If set to true a different internal structure is used,"
                        + " which buffers consecutive calls to write(). The update() function must then be called to trigger"
                        + " final outputting of the accumulated sequence of data.")
                .assignmentOptional().defaultValue(false)
                .init()
                .commit();
    }

    public Output(Hash config) {
        super(config);
        if (config.has("enableAppendMode"))
            m_appendModeEnabled = config.get("enableAppendMode");
    }

    public abstract void write(T object);
}
