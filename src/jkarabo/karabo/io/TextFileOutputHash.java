/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.io;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import static java.nio.file.StandardOpenOption.CREATE_NEW;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;
import karabo.util.Configurator;
import karabo.util.Hash;
import karabo.util.KARABO_CLASSINFO;
import karabo.util.Schema;
import karabo.util.TextSerializerHash;
import karabo.util.vectors.VectorHash;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
@KARABO_CLASSINFO(classId = "TextFile", version = "1.0")
public class TextFileOutputHash extends OutputHash {

    private Path m_filename = null;
    private String m_writeMode = null;
    private TextSerializerHash m_serializer;
    private VectorHash m_sequenceBuffer;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(OutputHash.class, TextFileOutputHash.class);
        //System.out.println("TextFileOutputHash class static registration");
    }

    public static void expectedParameters(Schema expected) {

        PATH_ELEMENT(expected).key("filename")
                .description("Name of the file to be written")
                .displayedName("Filename")
                .assignmentMandatory()
                .commit();

        STRING_ELEMENT(expected).key("writeMode")
                .description("Defines the behaviour in case of already existent file")
                .displayedName("Write Mode")
                .options("exclusive, truncate")
                .assignmentOptional().defaultValue("truncate")
                .commit();

        CHOICE_ELEMENT(expected).key("format")
                .displayedName("Format")
                .description("Select the format which should be used to interprete the data")
                .appendNodesOfConfigurationBase(TextSerializerHash.class)
                .assignmentOptional().noDefaultValue()
                .commit();
    }

    public TextFileOutputHash(Hash config) {
        super(config);
        m_filename = FileSystems.getDefault().getPath((String) config.get("filename"));
        if (config.has("writeMode")) {
            m_writeMode = config.get("writeMode");
        } else {
            m_writeMode = "truncate";
        }
        if (config.has("format")) {
            m_serializer = TextSerializerHash.createChoice(TextSerializerHash.class, "format", config);
        } else {
            guessAndSetFormat();
        }
    }

    @Override
    public void write(Hash data) {
        if (m_appendModeEnabled) {
            m_sequenceBuffer.add(data);
        } else {
            try {
                String archive = m_serializer.save(data);
                writeFile(archive);
            } catch (IOException ex) {
                Logger.getLogger(BinaryFileOutputHash.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
    }

    @Override
    public void update() {
        if (m_appendModeEnabled) {
            try {
                String archive = m_serializer.save(m_sequenceBuffer);
                writeFile(archive);
            } catch (IOException ex) {
                Logger.getLogger(TextFileOutputHash.class.getName()).log(Level.SEVERE, null, ex);
            }
            m_sequenceBuffer.clear();
        }
    }

    public void guessAndSetFormat() {
        ArrayList<String> keys = Configurator.getRegisteredClasses(TextSerializerHash.class);
        String fileName = m_filename.getFileName().toString();
        String[] tokens = fileName.split("\\.(?=[^\\.]+$)"); // get 2 tokens
        String extension = tokens[1].toLowerCase();
        for (String key : keys) {
            String lkey = key.toLowerCase();
            if (lkey.equals(extension)) {
                m_serializer = TextSerializerHash.create(TextSerializerHash.class, key);
                return;
            }
        }
        throw new UnsupportedOperationException("Can not interprete extension: \"" + extension + "\"");
    }

    public void writeFile(String buffer) throws IOException {
        String filename = m_filename.toString();
        if (null != m_writeMode) {
            switch (m_writeMode) {
                case "exclusive": {
                    try (OutputStream out = Files.newOutputStream(m_filename, CREATE_NEW)) {
                        out.write(buffer.getBytes());
                    }
                    break;
                }
                case "truncate": {
                    try (OutputStream out = Files.newOutputStream(m_filename)) {
                        out.write(buffer.getBytes());
                    }
                    break;
                }
            }
        }
    }
}
