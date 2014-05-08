/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorInteger;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorInt32Element extends VectorElement<VectorInteger> {

    public VectorInt32Element(Schema expected) {
        super(expected, new VectorInteger());
    }
}
