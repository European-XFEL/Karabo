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
public class VectorDouble extends ArrayList<Double> {

    public VectorDouble() {
        super();
    }

    public VectorDouble(int capacity) {
        super(capacity);
    }

    public VectorDouble(Collection<Double> c) {
        super(c);
    }

    public VectorDouble(String v) {
        super();
        String[] sa = v.split("[,]");
        for (String s : sa) {
            this.add(Double.parseDouble(s));
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
