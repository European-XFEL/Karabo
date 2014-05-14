/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.io.IOException;

/**
 *
 * @author esenov
 */
@KARABO_CLASSINFO(classId = "Xml", version = "1.0")
public class SchemaXmlSerializer extends TextSerializerSchema {

    private TextSerializerHash m_serializer;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(TextSerializerSchema.class, SchemaXmlSerializer.class);
        //System.out.println("SchemaXmlSerializer class static registration");
    }

    public static void expectedParameters(Schema expected) {
        HashXmlSerializer.expectedParameters(expected);
    }

    public SchemaXmlSerializer(Hash input) {
        m_serializer = TextSerializerHash.create(TextSerializerHash.class, "Xml", input);
    }

    @Override
    public String save(Schema object) throws IOException {
        String archive = object.getRootName() + ":";
        archive += m_serializer.save(object.getParameterHash());
        return archive;
    }

    @Override
    public Schema load(byte[] input) throws IOException {
        Schema schema = new Schema();
        String archive = new String(input);
        int idx = archive.indexOf(':');
        String rootName = archive.substring(0, idx);
        String hashArchive = archive.substring(idx+1);
        Hash hash = m_serializer.load(hashArchive);
        schema.setRootName(rootName);
        schema.setParameterHash(hash);
        schema.updateAliasMap();
        return schema;
    }
}
