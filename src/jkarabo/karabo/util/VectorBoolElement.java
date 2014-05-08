/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorBoolean;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorBoolElement extends VectorElement<VectorBoolean> {

    public VectorBoolElement(Schema expected) {
        super(expected, new VectorBoolean());
    }
}
