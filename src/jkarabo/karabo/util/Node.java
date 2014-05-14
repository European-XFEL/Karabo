/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.util.Objects;
import karabo.util.types.FromType;
import karabo.util.types.Types;
import karabo.util.vectors.VectorBoolean;
import karabo.util.vectors.VectorByte;
import karabo.util.vectors.VectorCharacter;
import karabo.util.vectors.VectorDouble;
import karabo.util.vectors.VectorFloat;
import karabo.util.vectors.VectorHash;
import karabo.util.vectors.VectorInteger;
import karabo.util.vectors.VectorLong;
import karabo.util.vectors.VectorNone;
import karabo.util.vectors.VectorShort;
import karabo.util.vectors.VectorString;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Node extends Object {

    private String m_key;
    private Attributes m_attributes;
    private Object m_value;
    
    {
        m_key = "";
        m_attributes = new Attributes();
        m_value = 0;
    }

    public Node() {
    }

    public <ValueType extends Object> Node(String key, ValueType value) {
        m_key = key;
        m_value = value;
    }

    public Node(Node other) {
        m_key = other.getKey();
        m_value = other.getValue();
        Attributes attrs = (Attributes) other.getAttributes();
        if (attrs == null) {
            m_attributes = (Attributes) null;
        } else {
            m_attributes = (Attributes) attrs.clone();
        }
    }

    /**
     * Get get associated with current node.
     * @return key
     */
    public String getKey() {
        return m_key;
    }

    /**
     *
     * @param key
     */
    void setKey(String key) {
        m_key = key;
    }

    /**
     * Get value associated with current node.
     * @param <ValueType> anything inherited from Object
     * @return value
     */
    public <ValueType extends Object> ValueType getValue() {
        return (ValueType) m_value;
    }

    /**
     * Get value associated with current node as Object
     * @return value
     */
    public Object getValueAsAny() {
        return m_value;
    }

    /**
     * Get value associated with current node as Object
     * @return value
     */
    public String getValueAsString() {
        return FromType.toString(m_value);
    }

    /**
     * Set current node value
     * @param value set as value associated with current node
     */
    public void setValue(Object value) {
        m_value = value;
    }

    public Types.ReferenceType getType() {
        return FromType.fromInstance(m_value);
    }

    public void setType(Types.ReferenceType targetType) {
        Types.ReferenceType sourceType = this.getType();
        if (sourceType == targetType) {
            return;
        }
        try {
            switch (targetType) {
                case BOOL:
                    m_value = (boolean) this.getValue();
                    break;
                case VECTOR_BOOL:
                    m_value = (VectorBoolean) this.getValue();
                    break;
                case CHAR:
                    m_value = (char) this.getValue();
                    break;
                case VECTOR_CHAR:
                    m_value = (VectorCharacter) this.getValue();
                    break;
                case INT8:
                    m_value = (byte) this.getValue();
                    break;
                case VECTOR_INT8:
                    m_value = (VectorByte) this.getValue();
                    break;
                case INT16:
                    m_value = (short) this.getValue();
                    break;
                case VECTOR_INT16:
                    m_value = (VectorShort) this.getValue();
                    break;
                case INT32:
                    m_value = (int) this.getValue();
                    break;
                case VECTOR_INT32:
                    m_value = (VectorInteger) this.getValue();
                    break;
                case INT64:
                    m_value = (long) this.getValue();
                    break;
                case VECTOR_INT64:
                    m_value = (VectorLong) this.getValue();
                    break;
                case FLOAT:
                    m_value = (float) this.getValue();
                    break;
                case VECTOR_FLOAT:
                    m_value = (VectorFloat) this.getValue();
                    break;
                case DOUBLE:
                    m_value = (double) this.getValue();
                    break;
                case VECTOR_DOUBLE:
                    m_value = (VectorDouble) this.getValue();
                    break;
                case STRING:
                    m_value = (String) this.getValue();
                    break;
                case VECTOR_STRING:
                    m_value = (VectorString) this.getValue();
                    break;
                case HASH:
                    m_value = (Hash) this.getValue();
                    break;
                case VECTOR_HASH:
                    m_value = (VectorHash) this.getValue();
                    break;
                case SCHEMA:
                    m_value = (Schema) this.getValue();
                    break;
                case NONE:
                    m_value = (CppNone) this.getValue();
                    break;
                case VECTOR_NONE:
                    m_value = (VectorNone) this.getValue();
                    break;
                default:
                    throw new RuntimeException("Casting of is not supported for target type");

            }
        } catch (Exception e) {
            throw new RuntimeException("Exception during type conversion: " + e);
        }
    }

    /**
     * Get all attributes associated with current node
     * @return Attributes object
     */
    public Attributes getAttributes() {
        return m_attributes;
    }

    /**
     * Set new Attributes object to be associated with current node.
     * @param attributes set Attributes object
     */
    public void setAttributes(Attributes attributes) {
        m_attributes = attributes;
    }

    public boolean hasAttribute(String attribute) {
        return m_attributes.containsKey(attribute);
    }

    /**
     * Get attribute by name from current node.
     * @param <AttributeType> anything inherited from Object
     * @param attribute attribute name
     * @return attribute value
     */
    public <AttributeType extends Object> AttributeType getAttribute(String attribute) {
        return (AttributeType) m_attributes.get(attribute);
    }

    public void setAttribute(String attribute, Object value) {
        m_attributes.put(attribute, value);
    }

    public boolean is(Class<? extends Object> type) {
        return m_value.getClass() == type;
    }

    @Override
    public int hashCode() {
        int hash = 3;
        hash = 17 * hash + Objects.hashCode(this.m_key);
        return hash;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Node other = (Node) obj;
        if (!Objects.equals(this.m_key, other.m_key)) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        StringBuilder result = new StringBuilder();
        String newLine = System.getProperty("line.separator");

        result.append(m_value.toString());
        return result.toString();
    }
}
