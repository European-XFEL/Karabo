/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
public class SlotElement extends SlotElementBase<SlotElement> {

    public SlotElement(Schema expected) {
        super(expected);
    }

    @Override
    public void beforeAddition() {
        m_node.setValue(m_child);
    }
    
}
