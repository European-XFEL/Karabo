/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.vectors.VectorString;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 * @param <Derived> anything that inherits from GenericElement
 */
public class GenericElement<Derived extends GenericElement> {

    protected Schema m_schema;
    protected Node m_node;
    
    {
        m_node = new Node("", 0);
    }

    public GenericElement(Schema expected) {
        m_schema = expected;
    }

    public Derived key(String name) {
        m_node.setKey(name);
        return (Derived) this;
    }

    public <AliasType extends Object> Derived alias(AliasType alias) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ALIAS, alias);
        if (m_node.getKey().isEmpty()) {
            throw new RuntimeException("You have to first assign a key to the expected parameter before you can set any alias");
        }
        m_schema.m_aliasToKey.put(alias.toString(), m_node.getKey());
        return (Derived) this;
    }

    public Derived tags(String tags, String sep) {
        VectorString vs = new VectorString();
        String[] sa = tags.split("[" + sep + "]");
        for (String s : sa) if (!"".equals(s)) vs.add(s);
        m_node.setAttribute(Schema.KARABO_SCHEMA_TAGS, vs);
        return (Derived) this;
    }

    public Derived tags(String tags) {
        return (Derived)tags(tags, " ,;");
    }

    public Derived displayedName(String name) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_DISPLAYED_NAME, name);
        return (Derived) this;
    }

    public Derived description(String description) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_DESCRIPTION, description);
        return (Derived) this;
    }

    public Derived observerAccess() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema.AccessLevel.OBSERVER.ordinal());
        return (Derived) this;
    }

    public Derived userAccess() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema.AccessLevel.USER.ordinal());
        return (Derived) this;
    }

    public Derived operatorAccess() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema.AccessLevel.OPERATOR.ordinal());
        return (Derived) this;
    }

    public Derived expertAccess() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema.AccessLevel.EXPERT.ordinal());
        return (Derived) this;
    }

    public Derived advanced() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema.AccessLevel.EXPERT.ordinal());
        return (Derived) this;
    }

    public Derived adminAccess() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema.AccessLevel.ADMIN.ordinal());
        return (Derived) this;
    }

    public void commit() {
        beforeAddition();
        if (m_schema == null) {
            throw new RuntimeException("Could not append element to non-initialized Schema object");
        } else {
            m_schema.addElement(m_node);
        }
    }

    public Node getNode() {
        return m_node;
    }

    protected void beforeAddition() {
    }
}
