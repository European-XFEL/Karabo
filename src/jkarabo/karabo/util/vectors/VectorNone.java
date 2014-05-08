/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.util.vectors;

import java.util.ArrayList;
import java.util.Collection;
import karabo.util.CppNone;

/**
 *
 * @author esenov
 */
public class VectorNone extends ArrayList<CppNone> {

    public VectorNone(int initialCapacity) {
        super(initialCapacity);
    }

    public VectorNone() {
    }

    public VectorNone(Collection<? extends CppNone> c) {
        super(c);
    }
    
}
