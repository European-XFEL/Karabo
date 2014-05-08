/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorCharacter;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorCharElement extends VectorElement<VectorCharacter> {

    public VectorCharElement(Schema expected) {
        super(expected, new VectorCharacter());
    }
}
