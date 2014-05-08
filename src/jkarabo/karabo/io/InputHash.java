/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.io;

import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
@KARABO_CLASSINFO(classId = "InputHash", version = "1.0")
public abstract class InputHash extends Input<Hash> {

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Input.class, InputHash.class);
        //System.out.println("InputHash class static registration");
    }

    public InputHash(Hash config) {
        super(config);
    }
    
}
