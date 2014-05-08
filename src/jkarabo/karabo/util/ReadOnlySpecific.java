/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class ReadOnlySpecific<Element extends GenericElement, ValueType extends Object> {

    private Element m_genericElement;

    public ReadOnlySpecific initialValue(ValueType initVal) {
        m_genericElement.getNode().setAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE, initVal);
        return this;
    }

    public ReadOnlySpecific initialValueFromString(String initVal) {
        m_genericElement.getNode().setAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE, initVal);
        return this;
    }

    public ReadOnlySpecific warnLow(ValueType value) {
        m_genericElement.getNode().setAttribute(Schema.KARABO_SCHEMA_WARN_LOW, value);
        return this;
    }

    public ReadOnlySpecific warnHigh(ValueType value) {
        m_genericElement.getNode().setAttribute(Schema.KARABO_SCHEMA_WARN_HIGH, value);
        return this;
    }

    public ReadOnlySpecific alarmLow(ValueType value) {
        m_genericElement.getNode().setAttribute(Schema.KARABO_SCHEMA_ALARM_LOW, value);
        return this;
    }

    public ReadOnlySpecific alarmHigh(ValueType value) {
        m_genericElement.getNode().setAttribute(Schema.KARABO_SCHEMA_ALARM_HIGH, value);
        return this;
    }

    public ReadOnlySpecific archivePolicy(Schema.ArchivePolicy value) {
        m_genericElement.getNode().setAttribute(Schema.KARABO_SCHEMA_ARCHIVE_POLICY, value);
        return this;
    }

    public void commit() {
        m_genericElement.commit();
    }

    protected ReadOnlySpecific() {
        m_genericElement = null;
    }

    void setElement(Element element) {
        m_genericElement = element;
    }
}
