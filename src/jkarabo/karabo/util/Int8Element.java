/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Int8Element extends SimpleElement<Byte> {

    public Int8Element(Schema expected) {
        super(expected, (byte) 0);
    }
}
