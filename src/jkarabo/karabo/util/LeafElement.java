/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorString;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class LeafElement<Derived extends LeafElement, ValueType extends Object> extends GenericElement<Derived> {

    private DefaultValue<Derived, ValueType> m_defaultValue = null;
    private ReadOnlySpecific<Derived, ValueType> m_readOnlySpecific = null;

    public LeafElement(Schema expected) {
        super(expected);
        m_defaultValue = new DefaultValue<>();
        m_defaultValue.setElement((Derived) this);
        m_readOnlySpecific = new ReadOnlySpecific<>();
        m_readOnlySpecific.setElement((Derived) this);
    }

    public Derived unit(Units.Unit unit) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_UNIT_ENUM, unit.ordinal());
        Units.Entry<String, String> names = Units.getUnit(unit);
        m_node.setAttribute(Schema.KARABO_SCHEMA_UNIT_NAME, names.name);
        m_node.setAttribute(Schema.KARABO_SCHEMA_UNIT_SYMBOL, names.symbol);
        return (Derived) this;
    }

    public Derived metricPrefix(Units.MetricPrefix metricPrefix) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_METRIC_PREFIX_ENUM, metricPrefix.ordinal());
        Units.Entry<String, String> names = Units.getMetricPrefix(metricPrefix);
        m_node.setAttribute(Schema.KARABO_SCHEMA_METRIC_PREFIX_NAME, names.name);
        m_node.setAttribute(Schema.KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.symbol);
        return (Derived) this;
    }

    public Derived allowedStates(String states, String sep) {
        VectorString vs = new VectorString();
        String[] sa = states.split("[" + sep + "]");
        for (String s : sa) if (!"".equals(s)) vs.add(s);
        m_node.setAttribute(Schema.KARABO_SCHEMA_ALLOWED_STATES, vs);
        return (Derived) this;
    }

    public Derived allowedStates(String states) {
        return allowedStates(states, " ,;");
    }

    public Derived assignmentMandatory() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.MANDATORY_PARAM.ordinal());
        return (Derived) this;
    }

    public DefaultValue<Derived, ValueType> assignmentOptional() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.OPTIONAL_PARAM.ordinal());
        return m_defaultValue;
    }

    public DefaultValue<Derived, ValueType> assignmentInternal() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.INTERNAL_PARAM.ordinal());
        return m_defaultValue;
    }

    public Derived init() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.INIT.ordinal());
        return (Derived) this;
    }

    public Derived reconfigurable() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.WRITE.ordinal());
        return (Derived) this;
    }

    public ReadOnlySpecific<Derived, ValueType> readOnly() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.READ.ordinal());
        // Set the assignment and defaults here, as the API would look strange to assign something to a read-only
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.OPTIONAL_PARAM.ordinal());
        m_node.setAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE, "0");
        return m_readOnlySpecific;
    }
}
