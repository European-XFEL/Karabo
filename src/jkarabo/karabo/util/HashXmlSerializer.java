/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.TextSerializer;
import java.io.IOException;
import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;

/**
 *
 * @author esenov
 */
@KARABO_CLASSINFO(classId = "Xml", version = "1.0")
public class HashXmlSerializer extends TextSerializer<Hash> {

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(TextSerializer.class, HashXmlSerializer.class);
        //System.out.println("HashXmlSerializer class static registration");
    }

    @Override
    public String save(Hash object) throws IOException {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Hash load(String archive) throws IOException {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

}
