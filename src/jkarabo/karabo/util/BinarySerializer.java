/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import karabo.util.ClassInfo;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 * @param <T> anything that inherits from Object
 */
@KARABO_CLASSINFO(classId = "BinarySerializer", version = "1.0")
public abstract class BinarySerializer<T extends Object> extends ClassInfo {

//    static {
//        KARABO_REGISTER_FOR_CONFIGURATION(BinarySerializer.class, BinarySerializer.class);
//        //System.out.println("BinarySerializer class static registration");
//    }

    public static void expectedParameters(Schema expected) {}
    
    public abstract ByteBuffer save(T object)  throws IOException;
    
    public ByteBuffer save(ArrayList<T> list) throws IOException {
        String result = "";
        for (T element : list)
            result += new String(save(element).array());
        return ByteBuffer.wrap(result.getBytes());
    }

    public abstract T load(byte[] archive, int offset, int nBytes) throws IOException;
    
    public T load(ByteBuffer archive) throws IOException {
        return load(archive.array(), archive.position(), archive.remaining());
    }
    
    public void load(ArrayList<T> objects, byte[] archive, int nBytes) throws IOException {
        if (objects == null)
            throw new RuntimeException("First argument cannot be null.");
        T object = load(archive, 0, nBytes);
        if (objects.isEmpty()) {
            objects.add(object);
        } else {
            objects.set(0, object);
        }
    }
    
    public void load(ArrayList<T> objects, ByteBuffer archive) throws IOException {
        load(objects, archive.array(), archive.capacity());
    }

}
