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
public class VectorLong extends ArrayList<Long> {

    public VectorLong() {
        super();
    }

    public VectorLong(int initialCapacity) {
        super(initialCapacity);
    }

    public VectorLong(Collection<Long> c) {
        super(c);
    }

    public VectorLong(String v) {
        super();
        String[] sa = v.split("[,]");
        for (String s : sa) {
            this.add(Long.parseLong(s));
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
        for (Long l : this) {
            result += l.toString() + ",";
        }
        return result.substring(0, result.length() - 1);
    }
}
