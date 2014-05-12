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
public class VectorInteger extends ArrayList<Integer> {

    public VectorInteger() {
        super();
    }

    public VectorInteger(int capacity) {
        super();
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(null);
        }
    }

    public VectorInteger(Collection<Integer> c) {
        super(c);
    }

    public VectorInteger(String v) {
        super();
        String[] sa = v.split("[,]");
        for (String s : sa) {
            this.add(Integer.parseInt(s));
        }
    }

    @Override
    public void ensureCapacity(int capacity) {
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(null);
        }
    }

    @Override
    public String toString() {
        String result = new String();
        for (Integer i : this) {
            result += i.toString() + ",";
        }
        return result.substring(0, result.length() - 1);
    }
}
