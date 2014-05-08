/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.types.Types;
import karabo.util.vectors.VectorString;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class ListElement extends GenericElement<ListElement> {

    private Schema.AssemblyRules m_parentSchemaAssemblyRules;
    private DefaultValue<ListElement, VectorString> m_defaultValue;

    public ListElement(Schema expected) {
        super(expected);
        m_parentSchemaAssemblyRules = expected.getAssemblyRules();
        if (m_defaultValue == null) {
            m_defaultValue = new DefaultValue<>();
        }
        m_defaultValue.setElement(this);
    }

    public ListElement min(int minNumNodes) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MIN, minNumNodes);
        return this;
    }

    public ListElement max(int maxNumNodes) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_MAX, maxNumNodes);
        return this;
    }

    public <ConfigurationBase> ListElement appendNodesOfConfigurationBase(Class<ConfigurationBase> configBase) {
        // Create an empty Hash as value of this choice node if not there yet
        if (m_node.getType() != Types.ReferenceType.HASH) {
            m_node.setValue(new Hash());
        }
        // Retrieve reference for filling
        Hash choiceOfNodes = m_node.getValue();

        VectorString nodeNames = Configurator.getRegisteredClasses(configBase);
        for (int i = 0; i < nodeNames.size(); ++i) {
            String nodeName = nodeNames.get(i);
            Schema schema = Configurator.getSchema(configBase, nodeName, m_parentSchemaAssemblyRules);
            Node node = choiceOfNodes.set(nodeName, schema.getParameterHash());
            node.setAttribute(Schema.KARABO_SCHEMA_CLASS_ID, nodeName);
            node.setAttribute(Schema.KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
            node.setAttribute(Schema.KARABO_SCHEMA_NODE_TYPE, Schema.NodeType.NODE.ordinal());
            node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.INIT_READ_WRITE.ordinal());
        }
        return this;
    }

    public ListElement appendAsNode(Class<? extends ClassInfo> type, String nodeName) {
        try {
            // Create an empty Hash as value of this choice node if not there yet
            if (m_node.getType() != Types.ReferenceType.HASH) {
                m_node.setValue(new Hash());
            }
            // Retrieve reference for filling
            Hash choiceOfNodes = m_node.getValue();

            // Get default classId
            ClassInfo ci = ClassInfo.classInfo(type);
            String classId = ci.getClassId();
            
            // Simply append the expected parameters of T to current node
            if (nodeName.isEmpty()) {
                nodeName = classId;
            }
            Schema schema = new Schema(nodeName, m_parentSchemaAssemblyRules);
            //T._KARABO_SCHEMA_DESCRIPTION_FUNCTION(schema);
            Method m = type.getMethod("expectedParameters", Schema.class);
            m.invoke(null, schema);
            Node node = choiceOfNodes.set(nodeName, schema.getParameterHash());
            node.setAttribute(Schema.KARABO_SCHEMA_CLASS_ID, classId);
            node.setAttribute(Schema.KARABO_SCHEMA_DISPLAY_TYPE, classId);
            node.setAttribute(Schema.KARABO_SCHEMA_NODE_TYPE, Schema.NodeType.NODE.ordinal());
            node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.INIT_READ_WRITE.ordinal());
            return this;
        } catch (NoSuchMethodException | SecurityException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex) {
            Logger.getLogger(ListElement.class.getName()).log(Level.SEVERE, null, ex);
        }
        return null;
    }

    public <T extends ClassInfo> ListElement appendAsNode(Class<T> type) {
        return appendAsNode(type, "");
    }

    /**
     * The <b>assignmentMandatory</b> method serves for setting up a mode that
     * requires the value of the element always being specified. No default
     * value is possible.
     *
     * @return reference to the Element (to allow method's chaining)
     */
    public ListElement assignmentMandatory() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.MANDATORY_PARAM.ordinal());
        return this;
    }

    public DefaultValue<ListElement, VectorString> assignmentOptional() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.OPTIONAL_PARAM.ordinal());
        return m_defaultValue;
    }

    @Override
    protected void beforeAddition() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.INIT_READ_WRITE.ordinal());
        m_node.setAttribute(Schema.KARABO_SCHEMA_NODE_TYPE, Schema.NodeType.LIST_OF_NODES.ordinal());
    }
}