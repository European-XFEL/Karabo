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
@KARABO_CLASSINFO(classId = "BinarySerializerHash", version = "1.0")
public abstract class BinarySerializerHash extends BinarySerializer<Hash> {

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(BinarySerializerHash.class, BinarySerializerHash.class);
        //System.out.println("BinarySerializerHash class static registration");
    }

    public static void expectedParameters(Schema expected) {
    }

}
