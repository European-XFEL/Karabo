/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.io;

import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;
import karabo.util.Configurator;
import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;
import karabo.util.TextSerializerHash;
import karabo.util.TextSerializerSchema;

/**
 *
 * @author Sergey Esenov <serguei.essenov@xfel.eu>
 */
@KARABO_CLASSINFO(classId = "TextFile", version = "1.0")
public class TextFileInputSchema extends InputSchema {

    private final Path m_filename;
    private TextSerializerSchema m_serializer;
    private ArrayList<Schema> m_sequenceBuffer;
    
    static {
        KARABO_REGISTER_FOR_CONFIGURATION(InputSchema.class, TextFileInputSchema.class);
        //System.out.println("TextFileInputSchema class static registration");
    }

    public static void expectedParameters(Schema expected) {

        PATH_ELEMENT(expected).key("filename")
                .description("Name of the file to be read")
                .displayedName("Filename")
                .assignmentMandatory()
                .commit();

        CHOICE_ELEMENT(expected).key("format")
                .displayedName("Format")
                .description("Select the format which should be used to interprete the data")
                .appendNodesOfConfigurationBase(TextSerializerSchema.class)
                .assignmentOptional().noDefaultValue()
                .commit();
    }

    public TextFileInputSchema(Hash config) {
        super(config);
        m_filename = FileSystems.getDefault().getPath(".", (String) config.get("filename"));
        if (config.has("format")) {
            m_serializer = TextSerializerSchema.createChoice(TextSerializerSchema.class, "format", config);
        } else {
            guessAndSetFormat();
        }
        // Read file already here
        byte[] archive;
        try {
            archive = Files.readAllBytes(m_filename);
            if (archive == null) {
                throw new IOException("Failed to read archive: " + m_filename.toString());
            }
            if (m_sequenceBuffer == null) {
                m_sequenceBuffer = new ArrayList<Schema>();
            }
            m_serializer.load(m_sequenceBuffer, archive);
        } catch (IOException ex) {
            Logger.getLogger(TextFileInputHash.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    @Override
    public Schema read(int idx) {
        return m_sequenceBuffer.get(idx);
    }

    @Override
    public int size() {
        return m_sequenceBuffer.size();
    }

    private void guessAndSetFormat() {
        ArrayList<String> keys = Configurator.getRegisteredClasses(TextSerializerHash.class);
        String fileName = m_filename.getFileName().toString();
        String[] tokens = fileName.split("\\.(?=[^\\.]+$)"); // get 2 tokens
        String extension = tokens[1].toLowerCase();
        for (String key : keys) {
            String lkey = key.toLowerCase();
            if (lkey.equals(extension)) {
                m_serializer = TextSerializerSchema.create(TextSerializerSchema.class, key);
                return;
            }
        }
        throw new UnsupportedOperationException("Can not interprete extension: \"" + extension + "\"");
    }
}
