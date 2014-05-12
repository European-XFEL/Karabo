/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.util.vectors;

import java.util.ArrayList;
import java.util.Collection;
import karabo.util.types.ComplexDouble;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
public class VectorComplexDouble extends ArrayList<ComplexDouble> {
    public VectorComplexDouble() {
        super();
    }
    public VectorComplexDouble(int capacity) {
        super(capacity);
    }
    
    public VectorComplexDouble(Collection<ComplexDouble> c) {
        super(c); 
    }
    
    public VectorComplexDouble(String c) {
        super();
        String s = c.substring(2, c.length() - 2);
        System.out.println("*** VectorComplexDouble: " + s);
    }
}
