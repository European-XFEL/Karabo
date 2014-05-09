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
public class VectorCharacter extends ArrayList<Character> {

    public VectorCharacter() {
        super();
    }

    public VectorCharacter(int capacity) {
        super();
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(null);
        }
    }

    public VectorCharacter(Collection<Character> c) {
        super(c);
    }

    public VectorCharacter(String vecchar) {
        throw new RuntimeException("VectorCharacter(String vecchar) not implemented yet");
    }

    @Override
    public void ensureCapacity(int capacity) {
        super.ensureCapacity(capacity);
        while (this.size() < capacity) {
            this.add(null);
        }
    }
}
