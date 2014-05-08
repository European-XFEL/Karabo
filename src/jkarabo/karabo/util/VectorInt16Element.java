/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorShort;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorInt16Element extends VectorElement<VectorShort> {

    public VectorInt16Element(Schema expected) {
        super(expected, new VectorShort());
    }
}
