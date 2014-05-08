/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.io;

import karabo.util.KARABO_CLASSINFO;
import karabo.util.Hash;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 * 
 */
@KARABO_CLASSINFO(classId = "Input", version = "1.0")
public abstract class Input<T extends Object> extends AbstractInput {

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Input.class, Input.class);
        //System.out.println("Input class static registration");
    }

    public Input(Hash config) {
        super(config);
    }
    
    public abstract T read(int idx);
    
    public T read() {
        return read(0);
    }
    
    public abstract int size();
}
