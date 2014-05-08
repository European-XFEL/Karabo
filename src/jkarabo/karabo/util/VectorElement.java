/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.types.FromType;
import karabo.util.types.ToLiteral;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class VectorElement<ValueType extends Object> extends LeafElement<VectorElement<ValueType>, ValueType> {

    private Class<?> theType;
    
    public VectorElement(Schema expected, ValueType arg) {
        super(expected);
        theType = arg.getClass();
    }

    public VectorElement minSize(int value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MIN_SIZE, value);
        return this;
    }

    public VectorElement maxSize(int value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MAX_SIZE, value);
        return this;
    }

    @Override
    protected void beforeAddition() {

        m_node.setAttribute(Schema.KARABO_SCHEMA_NODE_TYPE, Schema.NodeType.LEAF.ordinal());
        m_node.setAttribute(Schema.KARABO_SCHEMA_LEAF_TYPE, Schema.LeafType.PROPERTY.ordinal());
        m_node.setAttribute(Schema.KARABO_SCHEMA_VALUE_TYPE, ToLiteral.to(FromType.from(theType)));

        if (!m_node.hasAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE)) {
            this.init(); // This is the default
        }
        if (!m_node.hasAttribute(Schema.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {

            //for init, reconfigurable elements - set default value of requiredAccessLevel to USER
            if (!m_node.hasAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE) || //init element 
                    (int)m_node.getAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE) == AccessType.INIT.ordinal() || //init element 
                    (int)m_node.getAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE) == AccessType.WRITE.ordinal()) { //reconfigurable element

                this.userAccess();

            } else { //else set default value of requiredAccessLevel to OBSERVER 
                this.observerAccess();
            }
        }
    }
}
