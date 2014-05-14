/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.XMLOutputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamReader;
import javax.xml.stream.XMLStreamWriter;
import javax.xml.stream.events.XMLEvent;
import karabo.util.types.ComplexDouble;
import karabo.util.types.ComplexFloat;
import karabo.util.types.FromLiteral;
import karabo.util.types.FromType;
import karabo.util.types.ToLiteral;
import karabo.util.types.Types.ReferenceType;
import karabo.util.vectors.VectorBoolean;
import karabo.util.vectors.VectorByte;
import karabo.util.vectors.VectorCharacter;
import karabo.util.vectors.VectorComplexDouble;
import karabo.util.vectors.VectorComplexFloat;
import karabo.util.vectors.VectorDouble;
import karabo.util.vectors.VectorFloat;
import karabo.util.vectors.VectorHash;
import karabo.util.vectors.VectorInteger;
import karabo.util.vectors.VectorLong;
import karabo.util.vectors.VectorNone;
import karabo.util.vectors.VectorShort;
import karabo.util.vectors.VectorString;

/**
 *
 * @author esenov
 */
@KARABO_CLASSINFO(classId = "Xml", version = "1.0")
public class HashXmlSerializer extends TextSerializerHash {

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(TextSerializerHash.class, HashXmlSerializer.class);
        //System.out.println("HashXmlSerializer class static registration");
    }

    private XMLStreamWriter writer;
    private XMLStreamReader xmlr;
    private int processCount = 0;
    private int step = 2;
    private String prefix = null;
    private boolean debug = false;

    private class ParsedTuple {

        private String key = null;
        private Attributes attributes = null;
        private ReferenceType type = null;
        private Object value = null;

        public ParsedTuple() {
        }

        public void setAttributes(Attributes attributes) {
            this.attributes = attributes;
        }

        public void setKey(String key) {
            this.key = key;
        }

        public void setType(ReferenceType type) {
            this.type = type;
        }

        public void setValue(Object value) {
            this.value = value;
        }

        public String getKey() {
            return key;
        }

        public Attributes getAttributes() {
            return attributes;
        }

        public ReferenceType getType() {
            return type;
        }

        public Object getValue() {
            return value;
        }

        public void clear() {
            key = null;
            attributes = null;
            type = null;
            value = null;
        }

        public boolean isEmpty() {
            if (key == null && attributes == null && type == null && value == null) {
                return true;
            }
            return false;
        }
    }

    public static void expectedParameters(Schema expected) {

        INT32_ELEMENT(expected)
                .key("indentation")
                .description("Set the indent characters for printing. Value -1: the most dense formatting without linebreaks. "
                        + "Value 0: no indentation, value 1/2/3: one/two/three space indentation. If not set, default is 2 spaces.")
                .displayedName("Indentation")
                .options("-1 0 1 2 3 4")
                .assignmentOptional().defaultValue(2)
                .expertAccess()
                .commit();

        BOOL_ELEMENT(expected)
                .key("writeDataTypes")
                .description("This flag controls whether to add data-type information to the generated XML string")
                .displayedName("Write data types")
                .assignmentOptional().defaultValue(true)
                .expertAccess()
                .commit();

        BOOL_ELEMENT(expected)
                .key("readDataTypes")
                .description("This flag controls whether to use any potentially existing data type information to do automatic casting into the described types")
                .displayedName("Read data types")
                .assignmentOptional().defaultValue(true)
                .expertAccess()
                .commit();

        BOOL_ELEMENT(expected)
                .key("insertXmlNamespace")
                .displayedName("Insert XML Namespace")
                .description("Flag toggling whether to insert or not an xmlns attribute")
                .assignmentOptional().defaultValue(false)
                .expertAccess()
                .commit();

        STRING_ELEMENT(expected)
                .key("xmlns")
                .description("Sets the default XML namespace")
                .displayedName("XML Namespace")
                .assignmentOptional().defaultValue("http://xfel.eu/config")
                .expertAccess()
                .commit();

        STRING_ELEMENT(expected)
                .key("prefix")
                .displayedName("Prefix")
                .description("Prefix flagging auxiliary constructs needed for serialization")
                .assignmentOptional().defaultValue("KRB_")
                .expertAccess()
                .commit();
    }

    public HashXmlSerializer(Hash input) {
        if (input.has("indentation")) {
            step = input.<Integer>get("indentation");
        }
        if (input.has("prefix")) {
            prefix = input.<String>get("prefix");
        }
    }

    @Override
    public String save(Hash object) throws IOException {
        ByteArrayOutputStream os = null;
        String result = null;
        try {
            XMLOutputFactory factory = XMLOutputFactory.newInstance();
            os = new ByteArrayOutputStream(4096);
            writer = factory.createXMLStreamWriter(os);
            processCount = 0;
            writer.writeStartDocument();
            if (step >= 0) {
                writer.writeCharacters("\n");
            }
            writer.writeStartElement("root");
            writer.writeAttribute(prefix + "Artificial", "");
            writer.writeAttribute(prefix + "Type", ToLiteral.to(ReferenceType.HASH));
            if (step >= 0) {
                writer.writeCharacters("\n");
            }
            recordElement(object);
            writer.writeEndElement();

            writer.flush();
            result = new String(os.toByteArray());
        } catch (XMLStreamException ex) {
            Logger.getLogger(HashXmlSerializer.class.getName()).log(Level.SEVERE, null, ex);
        } finally {
            try {
                if (os != null) {
                    os.close();
                }
            } catch (IOException ex) {
                Logger.getLogger(HashXmlSerializer.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
        return result;
    }

    @Override
    public Hash load(byte[] archive) throws IOException {
        InputStream fis = null;
        Hash hash = new Hash();
        try {
            XMLInputFactory factory = XMLInputFactory.newInstance();
            factory.setProperty(XMLInputFactory.IS_REPLACING_ENTITY_REFERENCES, Boolean.TRUE);
            factory.setProperty(XMLInputFactory.IS_SUPPORTING_EXTERNAL_ENTITIES, Boolean.FALSE);
            //set the IS_COALESCING property to true , if application desires to
            //get whole text data as one event.            
            factory.setProperty(XMLInputFactory.IS_COALESCING, Boolean.FALSE);

            fis = new ByteArrayInputStream(archive);
            xmlr = factory.createXMLStreamReader(fis);
            //processEventType(xmlr.getEventType());
            printStartDocument();
            while (xmlr.hasNext()) {
                xmlr.next();
                //processEventType(xmlr.getEventType());
                if (xmlr.isStartElement()) {
                    printStartElement();
                    processElement(hash);
                } else if (xmlr.hasText()) {
                    printText();
                } else if (xmlr.isEndElement()) {
                    printEndElement();
                } else if (xmlr.getEventType() == XMLEvent.PROCESSING_INSTRUCTION) {
                    printPI();
                } else if (xmlr.getEventType() == xmlr.COMMENT) {
                    printComment();
                } else if (xmlr.END_DOCUMENT == xmlr.getEventType()) {
                    //System.out.println("End of Document");
                } else {
                    throw new RuntimeException("TOP Wrong eventType=" + getEventTypeString(xmlr.getEventType()));
                }
            }

        } catch (XMLStreamException ex) {
            Logger.getLogger(HashXmlSerializer.class.getName()).log(Level.SEVERE, null, ex);
        } finally {
            try {
                fis.close();
            } catch (IOException ex) {
                Logger.getLogger(HashXmlSerializer.class.getName()).log(Level.SEVERE, null, ex);
            }
        }

        return hash.<Hash>get("root");
    }

    private String getEventTypeString(int eventType) {
        switch (eventType) {
            case XMLEvent.START_ELEMENT:
                return "START_ELEMENT";

            case XMLEvent.END_ELEMENT:
                return "END_ELEMENT";

            case XMLEvent.PROCESSING_INSTRUCTION:
                return "PROCESSING_INSTRUCTION";

            case XMLEvent.CHARACTERS:
                return "CHARACTERS";

            case XMLEvent.COMMENT:
                return "COMMENT";

            case XMLEvent.START_DOCUMENT:
                return "START_DOCUMENT";

            case XMLEvent.END_DOCUMENT:
                return "END_DOCUMENT";

            case XMLEvent.ENTITY_REFERENCE:
                return "ENTITY_REFERENCE";

            case XMLEvent.ATTRIBUTE:
                return "ATTRIBUTE";

            case XMLEvent.DTD:
                return "DTD";

            case XMLEvent.CDATA:
                return "CDATA";

            case XMLEvent.SPACE:
                return "SPACE";
        }
        return "UNKNOWN_EVENT_TYPE , " + eventType;
    }

    private void processEventType(int eventType) {
        System.out.println("EVENT TYPE(" + eventType + ") = " + getEventTypeString(eventType));
    }

    private void printStartDocument() {
        if (debug && xmlr.START_DOCUMENT == xmlr.getEventType()) {
            System.out.print("<?xml version=\"" + xmlr.getVersion() + "\"");
            if (xmlr.getCharacterEncodingScheme() != null) {
                System.out.print(" encoding=\"" + xmlr.getCharacterEncodingScheme() + "\"");
            }
            System.out.println("?>");
        }
    }

    private void printStartElement() {
        if (debug) {
            System.out.print("<" + xmlr.getName().toString());
            printAttributes();
            System.out.print(">");
        }
    }

    private void printEndElement() {
        if (debug) {
            System.out.print("</" + xmlr.getName().toString() + ">");
        }
    }

    private void printText() {
        if (debug) {
            System.out.print(xmlr.getText());
        }
    }

    private void printPI() {
        if (debug) {
            System.out.print("<?" + xmlr.getPITarget() + " " + xmlr.getPIData() + "?>");
        }
    }

    private void printComment() {
        if (debug) {
            System.out.print("<!--" + xmlr.getText() + "-->");
        }
    }

    private void printAttributes() {
        if (debug) {
            int count = xmlr.getAttributeCount();
            if (count > 0) {
                for (int i = 0; i < count; i++) {
                    System.out.print(" ");
                    System.out.print(xmlr.getAttributeName(i).toString());
                    System.out.print("=");
                    System.out.print("\"");
                    System.out.print(xmlr.getAttributeValue(i));
                    System.out.print("\"");
                }
            }

            count = xmlr.getNamespaceCount();
            if (count > 0) {
                for (int i = 0; i < count; i++) {
                    System.out.print(" ");
                    System.out.print("xmlns");
                    if (xmlr.getNamespacePrefix(i) != null) {
                        System.out.print(":" + xmlr.getNamespacePrefix(i));
                    }
                    System.out.print("=");
                    System.out.print("\"");
                    System.out.print(xmlr.getNamespaceURI(i));
                    System.out.print("\"");
                }
            }
        }
    }

    private void processElement(Hash input) throws IOException {
        ParsedTuple tuple = new ParsedTuple();
        try {
            tuple.setKey(xmlr.getName().toString());
            processAttributes(tuple);

            //System.out.println("*** processElement Hash ENTER : " + processCount++ + " ===> \"" + tuple.getKey() + "\"\ttype = " + tuple.getType());
            boolean endFlag = false;
            while (xmlr.hasNext()) {
                int eventType = xmlr.next();
                //processEventType(eventType);
                if (xmlr.isStartElement()) {
                    printStartElement();
                    if (tuple.getType() == ReferenceType.VECTOR_HASH) {
                        if (tuple.getValue() == null) {
                            tuple.setValue(new VectorHash());
                        }
                        processElement((VectorHash) tuple.getValue());
                        //System.out.println("---------- VectorHash is \n" + vectorHash);
                    } else if (tuple.getType() == ReferenceType.HASH) {
                        if (tuple.getValue() == null) {
                            tuple.setValue(new Hash());
                        }
                        processElement((Hash) tuple.getValue());
                        //System.out.println("---------- Hash is \n" + hash);
                    } else {
                        throw new RuntimeException("Wrong reference type " + tuple.getType());
                    }
                    continue;  // read next event, because last event was END_ELEMENT
                }
                if (xmlr.hasText()) {
                    printText();
                    String text = xmlr.getText();
                    text = text.trim();     // trim space characters
                    if (!text.isEmpty()) {
                        //System.out.println("\n\t==> XML text: " + text);
                        tuple.setValue(text);
                    }
                }
                if (xmlr.isEndElement()) {
                    printEndElement();
                    //System.out.println("\t** H END ELEMENT for " + xmlr.getName().toString() + " Tuple: \"" + tuple.getKey() + "\", " + tuple.getType());
                    if (tuple.getKey().equals(prefix + "Item")) {
                        continue;
                    }
                    if (tuple.getValue() != null) {
                        input.set(tuple.getKey(), tuple.getType(), tuple.getValue());
                        if (tuple.getAttributes() != null) {
                            input.setAttributes(tuple.getKey(), tuple.getAttributes());
                        }
                    }
                    break;
                }
                if (xmlr.getEventType() == XMLEvent.PROCESSING_INSTRUCTION) {
                    printPI();
                    //System.out.println("\tXML processing instruction");
                }
                if (xmlr.getEventType() == XMLEvent.COMMENT) {
                    printComment();
                    //System.out.println("\tXML Comment");
                }
            }
        } catch (XMLStreamException ex) {
            Logger.getLogger(HashXmlSerializer.class.getName()).log(Level.SEVERE, null, ex);
        }

        //System.out.println("*** processElement Hash EXIT : " + --processCount + " ===> \"" + tuple.getKey() + "\"\ttype = " + tuple.getType() + "\n\t*** input is ...\n" + input);
    }

    private void processElement(VectorHash input) throws IOException {
        Hash hash = null;
        ParsedTuple tuple = new ParsedTuple();
        try {
            tuple.setKey(xmlr.getName().toString());
            processAttributes(tuple);

            //System.out.println("*** processElement VectoHash ENTER : " + processCount++ + " ===> \"" + tuple.getKey() + "\"\ttype = " + tuple.getType());
            boolean endFlag = false;
            while (xmlr.hasNext()) {
                int eventType = xmlr.next();
                //processEventType(eventType);
                if (xmlr.isStartElement()) {
                    printStartElement();
                    assert (prefix + "Item").equals(tuple.getKey());
                    if (tuple.getValue() == null) {
                        tuple.setValue(new Hash());
                    }
                    processElement((Hash) tuple.getValue());
                    //System.out.println("------ VectorHash is \n" + input);
                    continue;
                }
                if (xmlr.hasText()) {
                    printText();
                    String text = xmlr.getText().trim();
                    if (!text.isEmpty()) {
                        throw new RuntimeException("Text area for VECTOR_HASH contains non-Space characters");
                    }
                }
                if (xmlr.isEndElement()) {
                    printEndElement();
                    assert (prefix + "Item").equals(tuple.getKey());
                    input.add((Hash) tuple.getValue());
                    break;
                }
                if (xmlr.getEventType() == XMLEvent.PROCESSING_INSTRUCTION) {
                    printPI();
                    //System.out.print("<?" + xmlr.getPITarget() + " " + xmlr.getPIData() + "?>");
                }
                if (xmlr.getEventType() == XMLEvent.COMMENT) {
                    printComment();
                    //System.out.print("<!--" + xmlr.getText() + "-->");
                }
            }
        } catch (XMLStreamException ex) {
            Logger.getLogger(HashXmlSerializer.class.getName()).log(Level.SEVERE, null, ex);
        }

        //System.out.println("*** processElement VectorHash EXIT : " + --processCount + " ===> \"" + tuple.getKey() + "\"\ttype = " + tuple.getType() + "\n*** input is ...\n" + input);
    }

    private void processAttributes(ParsedTuple tuple) throws IOException {
        Attributes attributes = null;
        ReferenceType type = null;
        int count = xmlr.getAttributeCount();
        if (count > 0) {
            for (int i = 0; i < count; i++) {
                String attribute = xmlr.getAttributeName(i).toString();
                if (attribute.equals(prefix + "Type")) {
                    tuple.setType(FromLiteral.from(xmlr.getAttributeValue(i)));
                    continue;
                }
                if (attributes == null) {
                    attributes = new Attributes();
                }
                String stringVal = xmlr.getAttributeValue(i);
                if (stringVal.isEmpty()) {
                    putAttribute(attributes, attribute, prefix + "STRING", "");
                } else {
                    // split using ':' separator
                    String[] sa = stringVal.split("[:]");
                    putAttribute(attributes, attribute, sa[0], sa[1]);
                }
            }
            if (attributes != null) {
                tuple.setAttributes(attributes);
            }
        }
    }

    private void putAttribute(Attributes attributes, String attribute, String krbtype, String value) throws IOException {
        if (!krbtype.startsWith(prefix)) {
            throw new IOException("Attribute value type should start from \"" + prefix + "\"");
        }
        ReferenceType attributeType = FromLiteral.from(krbtype.substring(prefix.length()));
        switch (attributeType) {
            case BOOL:
                attributes.set(attribute, Integer.parseInt(value) != 0);
                break;
            case VECTOR_BOOL:
                attributes.set(attribute, new VectorBoolean(value));
                break;
            case CHAR:
                attributes.set(attribute, value.charAt(0));
                break;
            case VECTOR_CHAR:
                attributes.set(attribute, new VectorCharacter(value));
                break;
            case INT8:
            case UINT8:
                attributes.set(attribute, value.getBytes()[0]);
                break;
            case VECTOR_INT8:
            case VECTOR_UINT8:
                attributes.set(attribute, new VectorByte(value));
                break;
            case INT16:
            case UINT16:
                attributes.set(attribute, Short.parseShort(value));
                break;
            case VECTOR_INT16:
            case VECTOR_UINT16:
                attributes.set(attribute, new VectorShort(value));
                break;
            case INT32:
            case UINT32:
                attributes.set(attribute, Integer.parseInt(value));
                break;
            case VECTOR_INT32:
            case VECTOR_UINT32:
                attributes.set(attribute, new VectorInteger(value));
                break;
            case INT64:
            case UINT64:
                attributes.set(attribute, Long.parseLong(value));
                break;
            case VECTOR_INT64:
            case VECTOR_UINT64:
                attributes.set(attribute, new VectorLong(value));
                break;
            case FLOAT:
                attributes.set(attribute, Float.parseFloat(value));
                break;
            case VECTOR_FLOAT:
                attributes.set(attribute, new VectorFloat(value));
                break;
            case DOUBLE:
                attributes.set(attribute, Double.parseDouble(value));
                break;
            case VECTOR_DOUBLE:
                attributes.set(attribute, new VectorDouble(value));
                break;
            case STRING:
                attributes.set(attribute, value);
                break;
            case VECTOR_STRING:
                attributes.set(attribute, new VectorString(value));
                break;
            case COMPLEX_FLOAT:
                attributes.set(attribute, new ComplexFloat(value));
                break;
            case VECTOR_COMPLEX_FLOAT:
                attributes.set(attribute, new VectorComplexFloat(value));
                break;
            case COMPLEX_DOUBLE:
                attributes.set(attribute, new ComplexDouble(value));
                break;
            case VECTOR_COMPLEX_DOUBLE:
                attributes.set(attribute, new VectorComplexDouble(value));
                break;
            case NONE:
                assert "None".equals(value);
                attributes.set(attribute, CppNone.getInstance());
                break;
            case VECTOR_NONE: {
                int count = new VectorString(value).size();
                VectorNone vn = new VectorNone();
                while (count-- > 0) {
                    vn.add(CppNone.getInstance());
                }
                attributes.set(attribute, vn);
                break;
            }
            case HASH:
            case VECTOR_HASH:
            case SCHEMA:
            default:
                throw new RuntimeException("Unsupported type " + attributeType);
        }
    }

    private String indent(int counter) {
        String indentation = new String();
        if (step <= 0) {
            return indentation;
        }
        int count = counter * step;
        while (count > 0) {
            indentation += " ";
            count--;
        }
        return indentation;
    }

    private void recordElement(Hash hash) throws XMLStreamException {
        String indentation = indent(++processCount);
        for (Node node : hash.values()) {
            if (step >= 0) {
                writer.writeCharacters(indentation);
            }

            writer.writeStartElement(node.getKey());
            ReferenceType type = node.getType();
            recordAttributes(node.getAttributes(), type);
            if (type == ReferenceType.HASH) {
                if (step >= 0) {
                    writer.writeCharacters("\n");
                }
                recordElement(node.<Hash>getValue());
                if (step >= 0) {
                    writer.writeCharacters(indentation);
                }
            } else if (type == ReferenceType.VECTOR_HASH) {
                if (step >= 0) {
                    writer.writeCharacters("\n");
                }
                recordElement(node.<VectorHash>getValue());
                if (step >= 0) {
                    writer.writeCharacters(indentation);
                }
            } else {
                writer.writeCharacters(FromType.toString(node.getValueAsAny()));
            }
            writer.writeEndElement();

            if (step >= 0) {
                writer.writeCharacters("\n");
            }
        }
        processCount--;
    }

    private void recordElement(VectorHash vectorHash) throws XMLStreamException {
        String indentation = indent(++processCount);
        for (Hash hash : vectorHash) {
            if (step >= 0) {
                writer.writeCharacters(indentation);
            }

            writer.writeStartElement(prefix + "Item");
            if (step >= 0) {
                writer.writeCharacters("\n");
            }
            recordElement(hash);
            if (step >= 0) {
                writer.writeCharacters(indentation);
            }
            writer.writeEndElement();

            if (step >= 0) {
                writer.writeCharacters("\n");
            }
        }
        processCount--;
    }

    private void recordAttributes(Attributes attributes, ReferenceType type) throws XMLStreamException {

        for (Map.Entry<String, Object> e : attributes.entrySet()) {
            String key = e.getKey();
            Object attribute = e.getValue();
            ReferenceType attributeType = FromType.fromInstance(attribute);
            String attributeStringType = prefix + ToLiteral.to(attributeType);
            if (attributeType == ReferenceType.HASH || attributeType == ReferenceType.VECTOR_HASH) {
                throw new RuntimeException("Hash or VectorHash object as attribute value are not allowed.");
            }
            String attributeStringValue = FromType.toString(attribute);
            writer.writeAttribute(key, attributeStringType + ":" + attributeStringValue);
        }
        writer.writeAttribute(prefix + "Type", ToLiteral.to(type));
    }
}
