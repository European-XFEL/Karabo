/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Int32Element extends SimpleElement<Integer> {

    public Int32Element(Schema expected) {
        super(expected, (Integer) 0);
    }
}
