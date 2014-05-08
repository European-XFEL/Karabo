/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class DefaultValue<Element extends GenericElement, ValueType extends Object> {

    private Element m_genericElement;

    public Element defaultValue(ValueType defVal) {
        m_genericElement.getNode().setAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE, defVal);
        return m_genericElement;
    }

    public Element defaultValueFromString(String defVal) {
        m_genericElement.getNode().setAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE, defVal);

        return m_genericElement;
    }

    public Element noDefaultValue() {
        return m_genericElement;
    }

    protected DefaultValue() {
        m_genericElement = null;
    }

    void setElement(Element element) {
        m_genericElement = element;
    }
}
