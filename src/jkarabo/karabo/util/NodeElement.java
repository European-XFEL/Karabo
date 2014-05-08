/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class NodeElement extends GenericElement<NodeElement> {

    private Schema.AssemblyRules m_parentSchemaAssemblyRules;

    public NodeElement(Schema expected) {
        super(expected);
        m_parentSchemaAssemblyRules = expected.getAssemblyRules();
        m_node.setValue(new Hash());
    }

    public <ConfigurableClass extends ClassInfo> NodeElement appendParametersOfConfigurableClass(Class<ConfigurableClass> configurableClass, String classId) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_CLASS_ID, classId);
        m_node.setAttribute(Schema.KARABO_SCHEMA_DISPLAY_TYPE, ConfigurableClass.classInfo(configurableClass).getClassId());
        // Assemble schema (taking into account base classes, etc.) and append to node
        Schema schema = Configurator.getSchema(configurableClass, classId, m_parentSchemaAssemblyRules);
        // The produced schema will be rooted with classId, we however want to put its children
        // under the defined key and ignore the classId root node
        m_node.setValue(schema.getParameterHash());
        return this;
    }

    public <T> NodeElement appendParametersOf(Class<T> cls) {
        // Simply append the expected parameters of T to current node
        Schema schema = new Schema("dummyRoot", m_parentSchemaAssemblyRules);
        Class[] cArg = new Class[1];
        cArg[0] = Schema.class;
        try {
            Method m = cls.getDeclaredMethod("expectedParameters", cArg);
            m.invoke(null, schema);
            m_node.setValue(schema.getParameterHash());
            return this;
        } catch (NoSuchMethodException | SecurityException | IllegalAccessException | IllegalArgumentException ex) {
            Logger.getLogger(Configurator.class.getName()).log(Level.SEVERE, null, ex);
            throw new RuntimeException("Exception while invoking \"expectedParameters\" : " + ex);
        } catch (InvocationTargetException ex) {
            Logger.getLogger(NodeElement.class.getName()).log(Level.SEVERE, null, ex);
            throw new RuntimeException("Exception while invoking \"expectedParameters\" : " + ex);
        }
    }

    @Override
    protected void beforeAddition() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE, AccessType.INIT_READ_WRITE.ordinal());
        m_node.setAttribute(Schema.KARABO_SCHEMA_NODE_TYPE, Schema.NodeType.NODE.ordinal());
    }
}