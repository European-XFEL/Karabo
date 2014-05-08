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
public class OverwriteElement {

    private Schema m_schema;
    private Node m_node;

    public OverwriteElement(Schema expected) {
        m_schema = expected;
    }

    public OverwriteElement key(String name) {
        m_node = m_schema.getParameterHash().find(name);
        if (m_node == null) {
            throw new RuntimeException("Key \"" + name + "\" was not set before, thus can not be overwritten.");
        }
        return this;
    }

    public OverwriteElement setNewDisplayedName(String name) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_DISPLAYED_NAME, name);
        return this;
    }

    public OverwriteElement setNewDescription(String description) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_DESCRIPTION, description);
        return this;
    }

    public <AliasType> OverwriteElement setNewAlias(AliasType alias) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ALIAS, alias);
        return this;
    }

    public <TagType> OverwriteElement setNewTag(TagType tag) {
        m_node.setAttribute("tag", tag);
        return this;
    }

    public OverwriteElement setNewAssignmentMandatory() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.MANDATORY_PARAM.ordinal());
        return this;
    }

    public OverwriteElement setNewAssignmentOptional() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.OPTIONAL_PARAM.ordinal());
        return this;
    }

    public OverwriteElement setNewAssignmentInternal() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.INTERNAL_PARAM.ordinal());
        return this;
    }

    public OverwriteElement setNowInit() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.INIT.ordinal());
        return this;
    }

    public OverwriteElement setNowReconfigurable() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.WRITE.ordinal());
        return this;
    }

    public OverwriteElement setNowReadOnly() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.READ.ordinal());
        return this;
    }

    public <ValueType> OverwriteElement setNewDefaultValue(ValueType value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE, value);
        return this;
    }

    public <ValueType> OverwriteElement setNewMinInc(ValueType value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MIN_INC, value);
        return this;
    }

    public <ValueType> OverwriteElement setNewMaxInc(ValueType value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MAX_INC, value);
        return this;
    }

    public <ValueType> OverwriteElement setNewMinExc(ValueType value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MIN_EXC, value);
        return this;
    }

    public <ValueType> OverwriteElement setNewMaxExc(ValueType value) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MAX_EXC, value);
        return this;
    }

    public OverwriteElement setNewOptions(String opts, String sep) {
        VectorString vs = new VectorString();
        String[] sa = opts.split("[" + sep + "]");
        for (String s : sa) if (!"".equals(s)) vs.add(s);
        m_node.setAttribute(Schema.KARABO_SCHEMA_OPTIONS, vs);
        return this;
    }

    public OverwriteElement setNewOptions(String opts) {
        return setNewOptions(opts, " ,;");
    }

    public OverwriteElement setNewOptions(VectorString opts) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_OPTIONS, opts);
        return this;
    }

    public OverwriteElement setNewUnit(Units.Unit unit) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_UNIT_ENUM, unit.ordinal());
        Units.Entry<String, String> names = Units.getUnit(unit);
        m_node.setAttribute(Schema.KARABO_SCHEMA_UNIT_NAME, names.name);
        m_node.setAttribute(Schema.KARABO_SCHEMA_UNIT_SYMBOL, names.symbol);
        return this;
    }

    public OverwriteElement setNewMetricPrefix(Units.MetricPrefix metricPrefix) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_METRIC_PREFIX_ENUM, metricPrefix.ordinal());
        Units.Entry<String, String> names = Units.getMetricPrefix(metricPrefix);
        m_node.setAttribute(Schema.KARABO_SCHEMA_METRIC_PREFIX_NAME, names.name);
        m_node.setAttribute(Schema.KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.symbol);
        return this;
    }

    /**
     * The <b>commit</b> method injects the element to the expected parameters
     * list. If not called the element is not usable. This must be called after
     * the element is fully defined.
     */
    public void commit() {
        // Does nothing, changes happened on existing node
    }
}
