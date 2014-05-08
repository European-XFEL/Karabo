/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorByte;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorInt8Element extends VectorElement<VectorByte> {

    public VectorInt8Element(Schema expected) {
        super(expected, new VectorByte());
    }
}
