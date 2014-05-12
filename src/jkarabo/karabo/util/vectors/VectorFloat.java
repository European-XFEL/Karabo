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
public class VectorFloat extends ArrayList<Float> {

    public VectorFloat() {
        super();
    }

    public VectorFloat(int capacity) {
        super();
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(null);
        }
    }

    public VectorFloat(Collection<Float> c) {
        super(c);
    }

    public VectorFloat(String v) {
        super();
        String[] sa = v.split("[,]");
        for (String s : sa) {
            this.add(Float.parseFloat(s));
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
