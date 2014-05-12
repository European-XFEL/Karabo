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

    public VectorCharacter(String v) {
        super();
        String[] sa = v.split("[,]");
        for (String s : sa) {
            this.add(s.charAt(0));
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
        for (Character ch : this) {
            result += ch.toString() + ",";
        }
        return result.substring(0, result.length() - 1);
    }
}
