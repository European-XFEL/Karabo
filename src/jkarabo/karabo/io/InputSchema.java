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
@KARABO_CLASSINFO(classId = "InputSchema", version = "1.0")
public abstract class InputSchema extends Input<Schema> {

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Input.class, InputSchema.class);
        //System.out.println("InputSchema class static registration");
    }

    public InputSchema(Hash config) {
        super(config);
    }
    
}
