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
@KARABO_CLASSINFO(classId = "OutputHash", version = "1.0")
public abstract class OutputHash extends Output<Hash> {

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Output.class, OutputHash.class);
        //System.out.println("OutputHash class static registration");
    }

    public OutputHash(Hash config) {
        super(config);
    }
    
}
