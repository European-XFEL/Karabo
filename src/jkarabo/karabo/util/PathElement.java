/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.types.ToLiteral;
import karabo.util.types.Types;
import karabo.util.vectors.VectorString;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class PathElement extends LeafElement<PathElement, String> {

    public PathElement(Schema expected) {
        super(expected);
    }
    
    /**
     * The <b>options</b> method specifies values allowed for the parameter.
     * @param opts A string with space separated values. The values are casted to the proper type.
     * @param sep  A separator symbols. Default values are " ,;"
     * @return reference to the PathElement
     */
    public PathElement options(String opts, String sep) {
        VectorString vs = new VectorString();
        String[] sa = opts.split("[" + sep + "]");
        for (String s : sa) if (!"".equals(s)) vs.add(s);
        m_node.setAttribute(Schema.KARABO_SCHEMA_OPTIONS, vs);
        return this;
    }
    
    public PathElement options(String opts) {
        return options(opts, " ,;");
    }
    /**
     * The <b>options</b> method specifies values allowed for this parameter. Each value is an element of the vector.
     * This function can be used when space cannot be used as a separator.
     * @param opts vector of strings. The values are casted to the proper type.
     * @return reference to the PathElement
     */
    public PathElement options(VectorString opts) {
        m_node.setAttribute(Schema.KARABO_SCHEMA_OPTIONS, opts);
        return this;
    }

    public PathElement isInputFile() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_DISPLAY_TYPE, "fileIn");
        return this;
    }

    public PathElement isOutputFile() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_DISPLAY_TYPE, "fileOut");
        return this;
    }

    public PathElement isDirectory() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_DISPLAY_TYPE, "directory");
        return this;
    }

    @Override
    protected void beforeAddition() {
        m_node.setAttribute(Schema.KARABO_SCHEMA_NODE_TYPE, Schema.NodeType.LEAF.ordinal());
        m_node.setAttribute(Schema.KARABO_SCHEMA_LEAF_TYPE, Schema.LeafType.PROPERTY.ordinal());
        m_node.setAttribute(Schema.KARABO_SCHEMA_VALUE_TYPE, ToLiteral.to(Types.ReferenceType.STRING));
        if (!m_node.hasAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE)) this.init(); // This is the default

        if (!m_node.hasAttribute(Schema.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) { 

            //for init, reconfigurable elements - set default value of requiredAccessLevel to USER
            if (!m_node.hasAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE) ||
                 (int)m_node.getAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE) == AccessType.INIT.ordinal() ||
                 (int)m_node.getAttribute(Schema.KARABO_SCHEMA_ACCESS_MODE) == AccessType.WRITE.ordinal() ) {         

                this.userAccess();   

            } else { //else set default value of requiredAccessLevel to OBSERVER 
               this.observerAccess();
            } 
        }
    }
    
}
