/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorDouble;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorDoubleElement extends VectorElement<VectorDouble> {

    public VectorDoubleElement(Schema expected) {
        super(expected, new VectorDouble());
    }
}
