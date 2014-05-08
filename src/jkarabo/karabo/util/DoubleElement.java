/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class DoubleElement extends SimpleElement<Double> {

    public DoubleElement(Schema expected) {
        super(expected, (double) 0);
    }
}
