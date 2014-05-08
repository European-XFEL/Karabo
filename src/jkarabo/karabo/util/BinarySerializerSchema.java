/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
@KARABO_CLASSINFO(classId = "BinarySerializerSchema", version = "1.0")
public abstract class BinarySerializerSchema extends BinarySerializer<Schema> {
    
    static {
        KARABO_REGISTER_FOR_CONFIGURATION(BinarySerializerSchema.class, BinarySerializerSchema.class);
        //System.out.println("BinarySerializerSchema class static registration");
    }
    
    public static void expectedParameters(Schema expected) {}
    
}
