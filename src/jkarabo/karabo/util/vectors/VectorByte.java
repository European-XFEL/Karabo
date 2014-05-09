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
public class VectorByte extends ArrayList<Byte> {

    public VectorByte() {
        super();
    }

    public VectorByte(int capacity) {
        super();
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(null);
        }
    }

    public VectorByte(Collection<Byte> c) {
        super(c);
    }

    public VectorByte(String c) {
        super(c.length());
        for (Byte b : c.getBytes()) {
            this.add(b);
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
