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
 * @author esenov
 */
@KARABO_CLASSINFO(classId = "TextFile", version = "1.0")
public class TextFileOutput<T extends Object> extends Output<T> {

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Output.class, TextFileOutput.class);
        //System.out.println("TextFileOutput class static registration");
    }
    public TextFileOutput(Hash config) {
        super(config);
    }

    @Override
    public void write(T object) {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }
    
}
