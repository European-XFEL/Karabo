/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class FloatElement extends SimpleElement<Float> {

    public FloatElement(Schema expected) {
        super(expected, (float) 0);
    }
}
