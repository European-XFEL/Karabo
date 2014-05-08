/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorString;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorStringElement extends VectorElement<VectorString> {

    public VectorStringElement(Schema expected) {
        super(expected, new VectorString());
    }
}
