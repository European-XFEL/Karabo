/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Int64Element extends SimpleElement<Long> {

    public Int64Element(Schema expected) {
        super(expected, (long) 0);
    }
}
