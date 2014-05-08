/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorLong;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorInt64Element extends VectorElement<VectorLong> {

    public VectorInt64Element(Schema expected) {
        super(expected, new VectorLong());
    }
}
