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

    @Override
    public void ensureCapacity(int capacity) {
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(null);
        }
    }
}
