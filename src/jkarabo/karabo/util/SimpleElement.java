/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.types.FromType;
import karabo.util.types.ToLiteral;
import karabo.util.vectors.VectorString;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class SimpleElement<ValueType extends Object> extends LeafElement<SimpleElement<ValueType>, ValueType> {

    private Class<?> theType;

    public SimpleElement(Schema expected, ValueType arg) {
        super(expected);
        theType = arg.getClass();
    }

    public SimpleElement options(String opts, String sep) {
        VectorString vs = new VectorString();
        String[] sa = opts.split("[" + sep + "]");
        for (String s : sa) if (!"".equals(s)) vs.add(s);
        m_node.setAttribute(Schema.KARABO_SCHEMA_OPTIONS, vs);
        return this;
    }

    public SimpleElement options(String opts) {
        return options(opts, " ,;");
    }

    public SimpleElement options(VectorString opts) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_OPTIONS, opts);
        return this;
    }

    public SimpleElement minInc(ValueType value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MIN_INC, value);
        return this;
    }

    public SimpleElement maxInc(ValueType value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MAX_INC, value);
        return this;
    }

    public SimpleElement minExc(ValueType value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MIN_EXC, value);
        return this;
    }

    public SimpleElement maxExc(ValueType value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MAX_EXC, value);
        return this;
    }

    @Override
    protected void beforeAddition() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_NODE_TYPE, Schema.NodeType.LEAF.ordinal());
        m_node.setAttribute(Schema.KARABO_SCHEMA_LEAF_TYPE, Schema.LeafType.PROPERTY.ordinal());
        m_node.setAttribute(Schema.KARABO_SCHEMA_VALUE_TYPE, ToLiteral.to(FromType.from(theType)));

        if (!m_node.hasAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE)) {
            init(); // This is the default
        }
        if (!m_node.hasAttribute(Schema.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {

            //for init, reconfigurable elements - set default value of requiredAccessLevel to USER
            if (!m_node.hasAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE)
                    || (int)m_node.getAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE) == AccessType.INIT.ordinal()
                    || (int)m_node.getAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE) == AccessType.WRITE.ordinal()) {

                userAccess();

            } else { //else set default value of requiredAccessLevel to OBSERVER 
                observerAccess();
            }
        }
        checkMinExcMaxExc();
        checkMinIncMaxInc();
    }

    private <ValueType> double toDouble(ValueType arg) {
        double d = 0.0;
        if (arg instanceof Float) {
            d = ((Float) arg).doubleValue();
        } else if (arg instanceof Double) {
            d = (Double) arg;
        } else if (arg instanceof Integer) {
            d = ((Integer) arg).doubleValue();
        } else if (arg instanceof Long) {
            d = ((Long) arg).doubleValue();
        } else if (arg instanceof Short) {
            d = ((Short) arg).doubleValue();
        }
        return d;
    }

    private void checkMinIncMaxInc() {
        if (m_node.hasAttribute(Schema.KARABO_SCHEMA_MIN_INC) && m_node.hasAttribute(Schema.KARABO_SCHEMA_MAX_INC)) {
            ValueType min = m_node.getAttribute(Schema.KARABO_SCHEMA_MIN_INC);
            ValueType max = m_node.getAttribute(Schema.KARABO_SCHEMA_MAX_INC);
            if (toDouble(min) > toDouble(max)) {
                throw new RuntimeException("Minimum value (" + min + ") is greater than maximum ("
                        + max + ") on parameter \"" + m_node.getKey() + "\"");
            }
        }
    }

    private void checkMinExcMaxExc() {
        // this is a default implementation valid for all integral types
        if (m_node.hasAttribute(Schema.KARABO_SCHEMA_MIN_EXC) && m_node.hasAttribute(Schema.KARABO_SCHEMA_MAX_EXC)) {
            ValueType min = m_node.getAttribute(Schema.KARABO_SCHEMA_MIN_EXC);
            ValueType max = m_node.getAttribute(Schema.KARABO_SCHEMA_MAX_EXC);
            if (toDouble(min) > toDouble(max)) {
                throw new RuntimeException("The open range: (" + min + "," + max
                        + ") is empty on parameter \"" + m_node.getKey() + "\"");
            }
        }
    }
}
