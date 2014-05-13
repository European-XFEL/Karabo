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
        String s = c;
        int b = -1;
        int e = -1;
        while ((b = s.indexOf('(')) != -1 && (e = s.indexOf(')')) != -1) {
            String cmpx = s.substring(b + 1, e - 1);
            String[] sa = cmpx.split("[,]");
            if (sa.length == 2) {
                this.add(new ComplexDouble(Double.parseDouble(sa[0]), Double.parseDouble(sa[1])));
            } else {
                this.add(new ComplexDouble(Double.parseDouble(sa[0]), 0.0));
            }
            s = s.substring(e + 1);
        }
    }

    @Override
    public String toString() {
        String result = new String();
        for (ComplexDouble c : this) {
            result += "(" + ((Double) c.re()).toString() + "," + ((Double) c.im()).toString() + "),";
        }
        return result.substring(0, result.length() - 1);
    }
}
