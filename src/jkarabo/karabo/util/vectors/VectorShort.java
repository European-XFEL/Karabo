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
public class VectorShort extends ArrayList<Short> {

    public VectorShort() {
        super();
    }

    public VectorShort(int capacity) {
        super(capacity);
    }

    public VectorShort(Collection<Short> c) {
        super(c);
    }

    public VectorShort(String v) {
        super();
        String[] sa = v.split("[,]");
        for (String s : sa) {
            this.add(Short.parseShort(s));
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
