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
public class ChoiceElement extends GenericElement<ChoiceElement> {

    private Schema.AssemblyRules m_parentSchemaAssemblyRules;
    private DefaultValue<ChoiceElement, String> m_defaultValue = new DefaultValue<>();

    public ChoiceElement(Schema expected) {
        super(expected);
        m_parentSchemaAssemblyRules = expected.getAssemblyRules();
        m_defaultValue.setElement(this);
    }

    public <ConfigurationBase> ChoiceElement appendNodesOfConfigurationBase(Class<ConfigurationBase> configurationBase) {
        // Create an empty Hash as value of this choice node if not there yet
        if (m_node.getType() != Types.ReferenceType.HASH) {
            m_node.setValue(new Hash());
        }
        // Retrieve reference for filling
        Hash choiceOfNodes = m_node.getValue();

        VectorString nodeNames = Configurator.getRegisteredClasses(configurationBase);
        //System.out.println("nodeNames are " + nodeNames.toString());
        for (String nodeName : nodeNames) {
            //System.out.println("appendNodesOfConfigurationBase " + configurationBase.getName() + " : nodeName = " + nodeName);
            Schema schema = Configurator.getSchema(configurationBase, nodeName, m_parentSchemaAssemblyRules);
            Node node = choiceOfNodes.set(nodeName, schema.getParameterHash());
            node.setAttribute(Schema.KARABO_SCHEMA_CLASS_ID, nodeName);
            node.setAttribute(Schema.KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
            node.setAttribute(Schema.KARABO_SCHEMA_NODE_TYPE, Schema.NodeType.NODE.ordinal());
            node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.INIT_READ_WRITE.ordinal());
        }
        return this;
    }

    public <T> ChoiceElement appendAsNode(Class<T> cls, String nodeName) {
        // Create an empty Hash as value of this choice node if not there yet
        if (m_node.getType() != Types.ReferenceType.HASH) {
            m_node.setValue(new Hash());
        }
        // Retrieve reference for filling
        Hash choiceOfNodes = m_node.getValue();

        // Simply append the expected parameters of T to current node
        ClassInfo ci = ClassInfo.classInfo(cls);
        String classId = ci.getClassId();

        if (nodeName.isEmpty()) {
            nodeName = classId;
        }
        Schema schema = new Schema(nodeName, m_parentSchemaAssemblyRules);
        Class[] cArg = new Class[1];
        cArg[0] = Schema.class;
        try {
            Method m = cls.getDeclaredMethod("expectedParameters", cArg);
            m.invoke(null, schema);
        } catch (NoSuchMethodException | SecurityException | IllegalAccessException | IllegalArgumentException ex) {
            Logger.getLogger(Configurator.class.getName()).log(Level.SEVERE, null, ex);
            throw new RuntimeException("Exception while invoking \"expectedParameters\" : " + ex);
        } catch (InvocationTargetException ex) {
            Logger.getLogger(ChoiceElement.class.getName()).log(Level.SEVERE, null, ex);
            throw new RuntimeException("Exception while invoking \"expectedParameters\" : " + ex);
        }
        //Schema schema = karabo::util::confTools::assembleSchema<T > (nodeName, m_parentSchemaAssemblyRules);
        Node node = choiceOfNodes.set(nodeName, schema.getParameterHash());
        node.setAttribute(Schema.KARABO_SCHEMA_CLASS_ID, classId);
        node.setAttribute(Schema.KARABO_SCHEMA_DISPLAY_TYPE, classId);
        node.setAttribute(Schema.KARABO_SCHEMA_NODE_TYPE, Schema.NodeType.NODE.ordinal());
        node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.INIT_READ_WRITE.ordinal());
        return this;
    }

    public <T> ChoiceElement appendAsNode(Class<T> cls) {
        return appendAsNode(cls, "");
    }

    /**
     * The <b>assignmentMandatory</b> method serves for setting up a mode that
     * requires the value of the element always being specified. No default
     * value is possible.
     *
     * @return reference to the Element (to allow method's chaining)
     */
    public ChoiceElement assignmentMandatory() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.MANDATORY_PARAM.ordinal());
        return this;
    }

    public DefaultValue<ChoiceElement, String> assignmentOptional() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT, Schema.AssignmentType.OPTIONAL_PARAM.ordinal());
        return m_defaultValue;
    }

    /**
     * The <b>init</b> method serves for setting up an access type property that
     * allows the element to be included in initial schema.
     *
     * @return reference to the Element (to allow method's chaining)
     */
    public ChoiceElement init() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.INIT.ordinal());
        return this;
    }

    /**
     * The <b>reconfigurable</b> method serves for setting up an access type
     * property that allows the element to be included in initial,
     * reconfiguration and monitoring schemas.
     *
     * @return reference to the Element (to allow method's chaining)
     */
    public ChoiceElement reconfigurable() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.WRITE.ordinal());
        return this;
    }

    @Override
    protected void beforeAddition() {
        if (!m_node.hasAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE)) {
            m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.INIT_READ_WRITE.ordinal());
        }
        m_node.setAttribute(Schema.KARABO_SCHEMA_NODE_TYPE, Schema.NodeType.CHOICE_OF_NODES.ordinal());
    }
}
