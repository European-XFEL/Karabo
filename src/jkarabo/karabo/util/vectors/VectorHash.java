/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.vectors;

import java.util.ArrayList;
import java.util.Collection;
import karabo.util.Hash;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorHash extends ArrayList<Hash> {

    public VectorHash() {
        super();
    }

    public VectorHash(int capacity) {
        super();
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(new Hash());
        }
    }

    public VectorHash(Collection<Hash> c) {
        super(c);
    }

    @Override
    public void ensureCapacity(int capacity) {
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(new Hash());
        }
    }
}
