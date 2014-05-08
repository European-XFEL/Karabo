/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class CharElement extends SimpleElement<Character> {

    public CharElement(Schema expected) {
        super(expected, (char) 0);
    }
}
