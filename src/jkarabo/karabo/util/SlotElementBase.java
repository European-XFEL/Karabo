/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.util;

import static karabo.util.Schema.*;
import karabo.util.vectors.VectorString;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 * @param <Derived> anything extended from SlotElementBase
 */
public abstract class SlotElementBase<Derived extends SlotElementBase> extends LeafElement<SlotElementBase<Derived>, Derived> {

    protected Hash m_child = new Hash();
    
    public SlotElementBase(Schema expected) {
        super(expected);
        m_node.setAttribute(KARABO_SCHEMA_ACCESS_MODE, karabo.util.AccessType.INIT_READ_WRITE.ordinal());
        m_node.setAttribute(KARABO_SCHEMA_NODE_TYPE, karabo.util.Schema.NodeType.NODE.ordinal());
        m_node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Slot"); // Reserved displayType for commands
                
        //default value of requiredAccessLevel for Slot element: USER
        m_node.setAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, karabo.util.Schema.AccessLevel.USER.ordinal());

        m_child.set("connectedSignals", 0);
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_DISPLAYED_NAME, "Connected Signals");
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_DESCRIPTION, "Signals already connected to this slot");
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, karabo.util.Schema.AccessLevel.EXPERT.ordinal());
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_ASSIGNMENT, karabo.util.Schema.AssignmentType.OPTIONAL_PARAM.ordinal());
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_NODE_TYPE, karabo.util.Schema.NodeType.LEAF.ordinal());
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_LEAF_TYPE, karabo.util.Schema.LeafType.COMMAND.ordinal());
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_VALUE_TYPE, karabo.util.types.ToLiteral.to(karabo.util.types.Types.ReferenceType.VECTOR_STRING));

        this.connectionsAreReconfigurable();
    }
    
    /**
     * The <b>allowedStates</b> method serves for setting up allowed states for the element
     * @param states A string describing list of possible states.
     * @param sep A separator symbol used for parsing previous argument for list of states
     * @return reference to the Element (to allow method's chaining)
     */
    @Override
    public final Derived allowedStates(String states, String sep) {
        VectorString vs = new VectorString();
        String[] sa = states.split("[" + sep + "]");
        for (String s : sa) if (!"".equals(s)) vs.add(s);
        m_node.setAttribute(KARABO_SCHEMA_ALLOWED_STATES, vs);
        return (Derived) this;
    }

    public final Derived connectionAssignmentIsMandatory() {
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_ASSIGNMENT, karabo.util.Schema.AssignmentType.MANDATORY_PARAM.ordinal());
        return (Derived) this;
    }

    public final Derived connectionAssignmentIsOptional() {
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_ASSIGNMENT, karabo.util.Schema.AssignmentType.OPTIONAL_PARAM.ordinal());
        return (Derived) this;
    }

    public final Derived connectionsAreNotReconfigurable() {
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_ACCESS_MODE, karabo.util.AccessType.INIT.ordinal());
        return (Derived) this;
    }

    public final Derived connectionsAreReconfigurable() {
        m_child.setAttribute("connectedSignals", KARABO_SCHEMA_ACCESS_MODE, karabo.util.AccessType.WRITE.ordinal());
        return (Derived) this;
    }

    @Override
    public abstract void beforeAddition();
    
}
