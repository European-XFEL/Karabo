/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.io;

import karabo.util.TextSerializer;
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

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
@KARABO_CLASSINFO(classId = "TextFile", version = "1.0")
public class TextFileInput<T extends Object> extends Input<T> {

    private TextSerializer<T> m_serializer;
    private final Path m_filename;
    private ArrayList<T> m_sequenceBuffer;
    
    static {
        KARABO_REGISTER_FOR_CONFIGURATION(Input.class, TextFileInput.class);
        //System.out.println("TextFileInput class static registration");
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
                .appendNodesOfConfigurationBase(TextSerializer.class)
                .assignmentOptional().noDefaultValue()
                .commit();
    }

    public TextFileInput(Hash config) throws IOException {
        super(config);
        m_filename = FileSystems.getDefault().getPath(".", (String) config.get("filename"));
        if (config.has("format")) {
            m_serializer = TextSerializer.createChoice(TextSerializer.class, "format", config);
        } else {
            guessAndSetFormat();
        }
        // Read file already here
        String archive = null;
        readFile(archive);
        if (archive == null)
            throw new IOException("Failed to read archive: " + m_filename.toString());
        m_serializer.load(m_sequenceBuffer, archive);
    }

    @Override
    public T read(int idx) {
        return m_sequenceBuffer.get(idx);
    }

    @Override
    public int size() {
        return m_sequenceBuffer.size();
    }

    private void guessAndSetFormat() {
        ArrayList<String> keys = Configurator.getRegisteredClasses(TextSerializer.class);
        String fileName = m_filename.getFileName().toString();
        String[] tokens = fileName.split("\\.(?=[^\\.]+$)"); // get 2 tokens
        String extension = tokens[1].toLowerCase();
        for (String key : keys) {
            String lkey = key.toLowerCase();
            if (lkey.equals(extension)) {
                m_serializer = TextSerializer.create(TextSerializer.class, key);
                return;
            }
        }
        throw new UnsupportedOperationException("Can not interprete extension: \"" + extension + "\"");
    }
    
    private void readFile(String buffer) {
        try {
            buffer =  new String(Files.readAllBytes(m_filename));
            // TODO:  not finished
        } catch (IOException ex) {
            Logger.getLogger(TextFileInput.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}
