/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.vectors;

import java.util.ArrayList;
import java.util.Collection;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorBoolean extends ArrayList<Boolean> {
    
    public VectorBoolean() {
        super();
    }
    
    public VectorBoolean(int capacity) {
        super();
        super.ensureCapacity(capacity);
        while(this.size() < capacity) this.add(null);
    }
    
    public VectorBoolean(Collection<Boolean> c) {
        super(c);
    }
    
    public VectorBoolean(String v) {
        super();
        String[] sa = v.split("[,]");
        for (String s : sa) {
            int i = Integer.parseInt(s);
            this.add(i != 0 ? true : false);
        }
    }
    
    @Override
    public void ensureCapacity(int capacity) {
        super.ensureCapacity(capacity);
        while(this.size() < capacity) this.add(null);
    }
    
    @Override
    public String toString() {
        String result = new String();
        for (boolean b : this) {
            result += b? "1" : "0";
            result += ",";
        }
        return result.substring(0, result.length() - 1);
    }
}
