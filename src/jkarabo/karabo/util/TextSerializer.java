/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import karabo.util.ClassInfo;
import karabo.util.KARABO_CLASSINFO;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 * @param <T> anything extended from Object
 */
@KARABO_CLASSINFO(classId = "TextSerializer", version = "1.0")
public abstract class TextSerializer<T extends Object> extends ClassInfo {

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(TextSerializer.class, TextSerializer.class);
        //System.out.println("TextSerializer class static registration");
    }

    public abstract String save(T object) throws IOException;

    public String save(ArrayList<T> objects) throws IOException {
        String result = "";
        for (T element : objects)
            result += save(element);
        return result;
    }

    public abstract T load(byte[] archive) throws IOException;
    
    public T load(String archive) throws IOException {
        return load(archive.getBytes());
    }
    
    public T load(StringBuffer archive) throws IOException {
        return load(archive.toString());
    }

    public T load(InputStream archive) throws IOException {
        return load(archive.toString()); // Creates a copy, but may be overridden for more performance
    }

    public T load(byte[] archive, int nBytes) throws IOException {
        return load(new String(archive, 0, nBytes));
    }

    public T load(char[] archive) throws IOException {
        return load(new String(archive));
    }

    public T load(char[] archive, int count) throws IOException {
        return load(new String(archive, 0, count));
    }

    public void load(ArrayList<T> objects, String archive) throws IOException {
        if (objects == null)
            throw new RuntimeException("First argument cannot be null.");
        T object = load(archive);
        if (objects.isEmpty()) {
            objects.add(object);
        } else {
            objects.set(0, object);
        }
    }

    public void load(ArrayList<T> objects, InputStream archive) throws IOException {
        this.load(objects, archive.toString());
    }

}
