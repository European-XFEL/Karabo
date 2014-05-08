/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.util.HashMap;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Attributes extends HashMap<String, Object> {
    
    public Attributes() {
    }

    public Attributes(String key1, Object val1) {
        this.put(key1, val1);
    }

    public Attributes(String key1, Object val1, String key2, Object val2) {
        this.put(key1, val1);
        this.put(key2, val2);
    }

    public Attributes(String key1, Object val1, String key2, Object val2,
            String key3, Object val3) {
        this.put(key1, val1);
        this.put(key2, val2);
        this.put(key3, val3);
    }

    public Attributes(String key1, Object val1, String key2, Object val2,
            String key3, Object val3, String key4, Object val4) {
        this.put(key1, val1);
        this.put(key2, val2);
        this.put(key3, val3);
        this.put(key4, val4);
    }

    public Attributes(String key1, Object val1, String key2, Object val2,
            String key3, Object val3, String key4, Object val4,
            String key5, Object val5) {
        this.put(key1, val1);
        this.put(key2, val2);
        this.put(key3, val3);
        this.put(key4, val4);
        this.put(key5, val5);
    }

    public Attributes(String key1, Object val1, String key2, Object val2,
            String key3, Object val3, String key4, Object val4,
            String key5, Object val5, String key6, Object val6) {
        this.put(key1, val1);
        this.put(key2, val2);
        this.put(key3, val3);
        this.put(key4, val4);
        this.put(key5, val5);
        this.put(key6, val6);
    }

    public Object set(String attr, Object val) {
        return put(attr, val);
    }
    
    public boolean has(String attr) {
        return containsKey(attr);
    }
}
