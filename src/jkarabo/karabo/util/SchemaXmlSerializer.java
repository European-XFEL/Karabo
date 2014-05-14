/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.util;

import java.io.IOException;

/**
 *
 * @author esenov
 */
@KARABO_CLASSINFO(classId = "Xml", version = "1.0")
public class SchemaXmlSerializer extends TextSerializer<Schema> {
    
    static {
        KARABO_REGISTER_FOR_CONFIGURATION(TextSerializer.class, SchemaXmlSerializer.class);
        //System.out.println("SchemaXmlSerializer class static registration");
    }

    @Override
    public String save(Schema object) throws IOException {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Schema load(byte[] archive) throws IOException {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }
}
