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
public class VectorString extends ArrayList<String> {

    public VectorString() {
        super();
    }

    public VectorString(int capacity) {
        super();
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(null);
        }
    }

    public VectorString(Collection<String> c) {
        super(c);
    }

    public VectorString(String v) {
        super();
        String[] sa = v.split("[,]");
        for (String s : sa) {
            this.add(s);
        }
    }

    @Override
    public void ensureCapacity(int capacity) {
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(null);
        }
    }
}
