/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class BoolElement extends SimpleElement<Boolean> {
    
    public BoolElement(Schema expected) {
        super(expected, true);
    }
}
