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
@KARABO_CLASSINFO(classId = "TextSerializerHash", version = "1.0")
public abstract class TextSerializerHash extends TextSerializer<Hash> {

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(TextSerializerHash.class, TextSerializerHash.class);
        //System.out.println("TextSerializerHash class static registration");
    }

    public static void expectedParameters(Schema expected) {
    }
}
