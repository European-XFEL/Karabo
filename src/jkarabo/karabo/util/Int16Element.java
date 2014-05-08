/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Int16Element extends SimpleElement<Short> {

    public Int16Element(Schema expected) {
        super(expected, (short) 0);
    }
}
