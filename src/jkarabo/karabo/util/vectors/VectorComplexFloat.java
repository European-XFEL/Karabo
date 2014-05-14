/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.util.vectors;

import java.util.ArrayList;
import java.util.Collection;
import karabo.util.types.ComplexFloat;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
public class VectorComplexFloat extends ArrayList<ComplexFloat>  {
    
    public VectorComplexFloat() {
        super();
    }

    public VectorComplexFloat(int capacity) {
        super(capacity);
    }

    public VectorComplexFloat(Collection<ComplexFloat> c) {
        super(c);
    }

    public VectorComplexFloat(String c) {
        super();
        String s = c;
        int b = -1;
        int e = -1;
        while ((b = s.indexOf('(')) != -1 && (e = s.indexOf(')')) != -1) {
            String cmpx = s.substring(b + 1, e - 1);
            String[] sa = cmpx.split("[,]");
            if (sa.length == 2) {
                this.add(new ComplexFloat(Float.parseFloat(sa[0]), Float.parseFloat(sa[1])));
            } else {
                this.add(new ComplexFloat(Float.parseFloat(sa[0]), 0.0f));
            }
            s = s.substring(e + 1);
        }
    }

    @Override
    public String toString() {
        String result = new String();
        for (ComplexFloat c : this) {
            result += c.toString() + ",";
        }
        return result.substring(0, result.length() - 1);
    }
}
