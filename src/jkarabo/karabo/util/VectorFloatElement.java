/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorFloat;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorFloatElement extends VectorElement<VectorFloat> {

    public VectorFloatElement(Schema expected) {
        super(expected, new VectorFloat());
    }
}
