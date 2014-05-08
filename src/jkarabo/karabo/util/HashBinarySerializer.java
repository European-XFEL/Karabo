/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import karabo.util.types.ComplexDouble;
import karabo.util.types.FromCppEnum;
import karabo.util.types.FromType;
import karabo.util.types.ToLiteral;
import karabo.util.types.Types;
import karabo.util.vectors.VectorBoolean;
import karabo.util.vectors.VectorByte;
import karabo.util.vectors.VectorComplexDouble;
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
@KARABO_CLASSINFO(classId = "Bin", version = "1.0")
public class HashBinarySerializer extends BinarySerializerHash {

    private static final ByteOrder byteOrder = ByteOrder.LITTLE_ENDIAN;

    static {
        KARABO_REGISTER_FOR_CONFIGURATION(BinarySerializerHash.class, HashBinarySerializer.class);
        //System.out.println("HashBinarySerializer class static registration");
    }

    public static void expectedParameters(Schema expected) {
    }

    public HashBinarySerializer(Hash input) {
    }

    @Override
    public ByteBuffer save(Hash object) throws IOException {
        ByteArrayOutputStream f = new ByteArrayOutputStream(4096);
        try {
            write(object, f);
        } catch (IOException ex) {
            Logger.getLogger(HashBinarySerializer.class.getName()).log(Level.SEVERE, null, ex);
            throw ex;
        }
        byte b[] = f.toByteArray();
        return ByteBuffer.wrap(b);
    }

    @Override
    public Hash load(byte[] archive, int offset, int nBytes) {
        ByteBuffer bb = ByteBuffer.wrap(archive, offset, nBytes);
        bb.order(byteOrder);
        Hash object = new Hash();
        try {
            read(object, bb);
        } catch (IOException ex) {
            Logger.getLogger(HashBinarySerializer.class.getName()).log(Level.SEVERE, null, ex);
        }
        return object;
    }

    private void write(Hash h, OutputStream os) throws IOException {
        writeSize(os, h.size());
        for (Node node : h.values()) {
            write(node, os);
        }
    }

    private void write(Node node, OutputStream os) throws IOException {
        writeKey(node.getKey(), os);
        writeType(node.getType(), os);
        write(node.getAttributes(), os);
        if (node.getType() == Types.ReferenceType.HASH) {
            write((Hash) node.getValue(), os);
        } else {
            write(node.getValueAsAny(), node.getType(), os);
        }
    }

    private void write(Attributes attributes, OutputStream os) throws IOException {
        //System.out.println("Start to write attributes. First number of attributes...");
        writeSize(os, attributes.size());
        for (Map.Entry<String, Object> entry : attributes.entrySet()) {
            writeKey(entry.getKey(), os);
            Object value = entry.getValue();
            Types.ReferenceType type = FromType.fromInstance(value);
            writeType(type, os);
            write(value, type, os);
        }
    }
    
    private void writeSize(OutputStream os, int size) throws IOException {
        writeSingleValue(os, size);
    }

    private void writeKey(String key, OutputStream os) throws IOException {
        //System.out.println("write key -> " + key + ", keylen = " + key.length());
        os.write((byte)key.length());
        os.write(key.getBytes());
    }

    private void writeType(Types.ReferenceType type, OutputStream os) throws IOException {
        //System.out.println("write type-> " + type + " as " + type.getValue());
        writeSingleValue(os, type.getValue());
    }

    private void write(Object any, Types.ReferenceType type, OutputStream os) throws IOException {
        switch (Types.category(type)) {
            case HASH: // BH: Can this ever happen, I think not!
            case SCHEMA:
            case SIMPLE:
                writeSingleValue(os, any, type);
                return;
            case VECTOR_HASH:
            case SEQUENCE:
                writeSequence(os, any, type);
                return;
            default:
                throw new IOException("Could not properly categorize value type \"" + ToLiteral.to(type) + "\" for writing to archive");
        }
    }

    private void writeSingleValue(OutputStream os, byte value) throws IOException {
        ByteBuffer bb = ByteBuffer.allocate(Byte.SIZE/8);
        bb.put(value);
        //System.out.println("write byte->  value = " + value);
        os.write(bb.array());
    }

    private void writeSingleValue(OutputStream os, short value) throws IOException {
        ByteBuffer bb = ByteBuffer.allocate(Short.SIZE/8);
        bb.order(byteOrder);
        bb.putShort(value);
        //System.out.println("write short->  value = " + value);
        os.write(bb.array());
    }

    private void writeSingleValue(OutputStream os, int value) throws IOException {
        ByteBuffer bb = ByteBuffer.allocate(Integer.SIZE/8);
        bb.order(byteOrder);
        bb.putInt(value);
        //System.out.println("write int->  value = " + value);
        os.write(bb.array());
    }

    private void writeSingleValue(OutputStream os, long value) throws IOException {
        ByteBuffer bb = ByteBuffer.allocate(Long.SIZE/8);
        bb.order(byteOrder);
        bb.putLong(value);
        //System.out.println("write long->  value = " + value);
        os.write(bb.array());
    }

    private void writeSingleValue(OutputStream os, float value) throws IOException {
        ByteBuffer bb = ByteBuffer.allocate(Float.SIZE/8);
        bb.order(byteOrder);
        bb.putFloat(value);
        //System.out.println("write float->  value = " + value);
        os.write(bb.array());
    }

    private void writeSingleValue(OutputStream os, double value) throws IOException {
        ByteBuffer bb = ByteBuffer.allocate(Double.SIZE/8);
        bb.order(byteOrder);
        bb.putDouble(value);
        //System.out.println("write double->  value = " + value);
        os.write(bb.array());
    }

    private void writeSingleValue(OutputStream os, boolean value) throws IOException {
        ByteBuffer bb = ByteBuffer.allocate(Byte.SIZE/8);
        bb.put(value ? (byte) 1 : (byte) 0);
        //System.out.println("write boolean->  value = " + value);
        os.write(bb.array());
    }

    private void writeSingleValue(OutputStream os, String value) throws IOException {
        //System.out.println("write String->  value.length = " + value.length() + " -- " + value);
        writeSize(os, value.length());
        os.write(value.getBytes());
    }

    private void writeSingleValue(OutputStream os, ComplexDouble value) throws IOException {
        //System.out.println("write String->  value.length = " + value.length() + " -- " + value);
        writeSingleValue(os, value.re());
        writeSingleValue(os, value.im());
    }

    private void writeSingleValue(OutputStream os, CppNone value) throws IOException {
        writeSize(os, (int)0);
    }

   private void writeSingleValue(OutputStream os, Schema value) throws IOException {
        Hash hash = new Hash();
        SchemaBinarySerializer serializer = new SchemaBinarySerializer(hash);
        ByteBuffer archive = serializer.save((Schema) value);
        if (archive == null) {
            throw new IOException("Failed to serialize Schema object.");
        }
        writeSize(os, archive.position());   // length
        os.write(archive.array());
    }

    private void writeSingleValue(OutputStream os, Object value, Types.ReferenceType type) throws IOException {
        switch (type) {
            case CHAR:
            case INT8:
            case UINT8:
                writeSingleValue(os, (byte) value);
                break;
            case INT16:
            case UINT16:
                writeSingleValue(os, (short) value);
                break;
            case INT32:
            case UINT32:
                writeSingleValue(os, (int) value);
                break;
            case INT64:
            case UINT64:
                writeSingleValue(os, (long) value);
                break;
            case FLOAT:
                writeSingleValue(os, (float) value);
                break;
            case DOUBLE:
                writeSingleValue(os, (double) value);
                break;
            case COMPLEX_DOUBLE:
                writeSingleValue(os, (ComplexDouble) value);
                break;
            case BOOL:
                writeSingleValue(os, (boolean) value);
                break;
            case STRING:
                writeSingleValue(os, (String) value);
                break;
            case HASH:
                write((Hash) value, os);
                break;
            case SCHEMA:
                writeSingleValue(os, (Schema) value);
                break;
            case NONE:
                writeSingleValue(os, (CppNone) value);
                break;
            default:
                throw new IOException("Encountered unknown data type whilst writing to binary archive");
        }

    }

    private void writeSequence(OutputStream os, VectorByte value) throws IOException {
        writeSize(os, value.size());
        for (byte e : value) {
            writeSingleValue(os, e);
        }
    }

    private void writeSequence(OutputStream os, VectorShort value) throws IOException {
        writeSize(os, value.size());
        for (short e : value) {
            writeSingleValue(os, e);
        }
    }

    private void writeSequence(OutputStream os, VectorInteger value) throws IOException {
        writeSize(os, value.size());
        for (int e : value) {
            writeSingleValue(os, e);
        }
    }

    private void writeSequence(OutputStream os, VectorLong value) throws IOException {
        writeSize(os, value.size());
        for (long e : value) {
            writeSingleValue(os, e);
        }
    }

    private void writeSequence(OutputStream os, VectorFloat value) throws IOException {
        writeSize(os, value.size());
        for (float e : value) {
            writeSingleValue(os, e);
        }
    }

    private void writeSequence(OutputStream os, VectorDouble value) throws IOException {
        writeSize(os, value.size());
        for (double e : value) {
            writeSingleValue(os, e);
        }
    }

    private void writeSequence(OutputStream os, VectorComplexDouble value) throws IOException {
        writeSize(os, value.size());
        for (ComplexDouble e : value) {
            writeSingleValue(os, e);
        }
    }

    private void writeSequence(OutputStream os, VectorHash value) throws IOException {
        writeSize(os, value.size());
        for (Hash e : value) {
            write(e, os);
        }
    }

    private void writeSequence(OutputStream os, VectorString value) throws IOException {
        writeSize(os, value.size());
        for (String e : value) {
            writeSingleValue(os, e);
        }
    }

    private void writeSequence(OutputStream os, VectorBoolean value) throws IOException {
        writeSize(os, value.size());
        for (boolean e : value) {
            writeSingleValue(os, e);
        }
    }

    private void writeSequence(OutputStream os, VectorNone value) throws IOException {
        writeSize(os, value.size());
        for (CppNone e : value) {
            writeSingleValue(os, e);
        }
    }

    private void writeSequence(OutputStream os, Object value, Types.ReferenceType type) throws IOException {
        switch (type) {
            case VECTOR_CHAR:
            case VECTOR_INT8:
            case VECTOR_UINT8:
                writeSequence(os, (VectorByte) value);
                break;
            case VECTOR_INT16:
            case VECTOR_UINT16:
                writeSequence(os, (VectorShort) value);
                break;
            case VECTOR_INT32:
            case VECTOR_UINT32:
                writeSequence(os, (VectorInteger) value);
                break;
            case VECTOR_INT64:
            case VECTOR_UINT64:
                writeSequence(os, (VectorLong) value);
                break;
            case VECTOR_FLOAT:
                writeSequence(os, (VectorFloat) value);
                break;
            case VECTOR_DOUBLE:
                writeSequence(os, (VectorDouble) value);
                break;
            case VECTOR_COMPLEX_DOUBLE:
                writeSequence(os, (VectorComplexDouble) value);
                break;
            case VECTOR_HASH:
                writeSequence(os, (VectorHash) value);
                break;
            case VECTOR_STRING:
                writeSequence(os, (VectorString) value);
                break;
            case VECTOR_BOOL:
                writeSequence(os, (VectorBoolean) value);
                break;
            case VECTOR_NONE:
                writeSequence(os, (VectorNone) value);
                break;
            default:
                throw new IOException("Encountered unknown array data type whilst writing to binary archive");
        }
    }

    private void read(Hash hash, ByteBuffer is) throws IOException {
        int size = readSize(is);
        for (int i = 0; i < size; i++) {
            String key = readKey(is);
            Types.ReferenceType type = readType(is);
            Attributes attributes = readAttributes(is);
            Object value;
            if (type == Types.ReferenceType.HASH) {
                value = new Hash();
                read((Hash) value, is);
            } else {
                value = read(type, is);
            }
            hash.set(key, value);
            hash.setAttributes(key, attributes);
        }
    }

    private Attributes readAttributes(ByteBuffer is) throws IOException {
        Attributes attributes = new Attributes();
        int size = readSize(is);
        for (int i = 0; i < size; i++) {
            String name = readKey(is);
            Types.ReferenceType type = readType(is);
            Object value = read(type, is);
            attributes.set(name, value);
        }
        return attributes;    
    }

    private Object read(Types.ReferenceType type, ByteBuffer is) throws IOException {
        Object value = null;
        switch (Types.category(type)) {
            case SCHEMA:
            case SIMPLE:
                value = readSingleValue(is, type);
                return value;
            case SEQUENCE:
                value = readSequence(is, type);
                return value;
            case HASH:
                value = new Hash();
                read((Hash) value, is);
                break;

            case VECTOR_HASH:
                value = new VectorHash();
                VectorHash result = (VectorHash) value;
                int size = readSize(is);
                for (int i = 0; i < size; ++i) {
                    Hash hash = new Hash();
                    read(hash, is);
                    result.add(hash);
                }
                break;
            default:
                throw new IOException("Could not properly categorize value \"" + ToLiteral.to(type) + "\" for reading from archive");
        }
        return value;
    }

    private int readSize(ByteBuffer is) {
        try {
            return is.getInt();
        } catch (BufferUnderflowException ex) {
            return 0;
        }
    }

    private String readKey(ByteBuffer is) throws IOException {
        int size = (int)is.get();
        byte[] dst = new byte[size];
        is.get(dst);
        String key = new String(dst);
        return key;
    }

    private Types.ReferenceType readType(ByteBuffer is) throws IOException {
        try {
            int cppType = is.getInt();
            return FromCppEnum.from(cppType);
        } catch (BufferUnderflowException ex) {
            throw new IOException("EOF");
        } catch (InterruptedException ex) {
            Logger.getLogger(HashBinarySerializer.class.getName()).log(Level.SEVERE, null, ex);
        }
        return Types.ReferenceType.UNKNOWN;
    }

    private String readStringValue(ByteBuffer is) throws IOException {
        int size = readSize(is);
        if (size == 0) {
            throw new IOException("EOF");
        }
        byte[] dst = new byte[size];
        is.get(dst);
        String s = new String(dst);
        //System.out.println("read String  ->  size = " + size + ", value = " + s);
        return s;
    }

    private boolean readBooleanValue(ByteBuffer is) throws IOException {
        if (is.remaining() == 0) {
            throw new IOException("EOF");
        }
        byte b = is.get();
        //System.out.println("read boolean ->  value = " + (b!=0));
        return (b != 0);
    }

    private byte readByteValue(ByteBuffer is) throws IOException {
        if (is.remaining() == 0) {
            throw new IOException("EOF");
        }
        byte b = is.get();
        //System.out.println("read byte    ->  value = " + b);
        return b;
    }

    private short readShortValue(ByteBuffer is) throws IOException {
        if (is.remaining() < Short.SIZE/8) {
            throw new IOException("EOF");
        }
        short s = is.getShort();
        //System.out.println("read short   ->  value = " + s);
        return s;
    }

    private int readIntValue(ByteBuffer is) throws IOException {
        if (is.remaining() < Integer.SIZE/8) {
            throw new IOException("EOF");
        }
        int i = is.getInt();
        //System.out.println("read int     ->  value = " + i);
        return i;
    }

    private long readLongValue(ByteBuffer is) throws IOException {
        if (is.remaining() < Long.SIZE/8) {
            throw new IOException("EOF");
        }
        long l = is.getLong();
        //System.out.println("read long    ->  value = " + l);
        return l;
    }

    private float readFloatValue(ByteBuffer is) throws IOException {
        if (is.remaining() < Float.SIZE/8) {
            throw new IOException("EOF");
        }
        float f = is.getFloat();
        //System.out.println("read float   ->  value = " + f);
        return f;
    }

    private double readDoubleValue(ByteBuffer is) throws IOException {
        if (is.remaining() < Double.SIZE/8) {
            throw new IOException("EOF");
        }
        double d = is.getDouble();
        //System.out.println("read double  ->  value = " + d);
        return d;
    }

    private ComplexDouble readComplexDoubleValue(ByteBuffer is) throws IOException {
        ComplexDouble cd = new ComplexDouble(readDoubleValue(is), readDoubleValue(is));
        //System.out.println("read ComplexDouble  ->  value = " + cd);
        return cd;
    }

    private Hash readHashValue(ByteBuffer is) throws IOException {
        Hash hash = new Hash();
        read(hash, is);
        return hash;
    }

    private Schema readSchemaValue(ByteBuffer is) throws IOException {
        Hash hash = new Hash();
        SchemaBinarySerializer serializer = new SchemaBinarySerializer(hash);
        // TODO Optimize this by reading directly from istream
        int size = readSize(is);
        byte[] buffer = new byte[size];
        is.get(buffer, 0, size);
        Schema schema = serializer.load(buffer, 0, size);
        return schema;
    }

    private CppNone readNoneValue(ByteBuffer is) throws IOException {
        if (is.remaining() < Integer.SIZE/8) {
            throw new IOException("EOF");
        }
        int size = readSize(is);
        assert size == 0;
        return CppNone.getInstance();
    }

    private Object readSingleValue(ByteBuffer is, Types.ReferenceType type) throws IOException {
        //System.out.println("readSingleValue: type is " + type + " == " + type.ordinal());
        switch (type) {
            case CHAR:
            case INT8:
            case UINT8:
                return (Object) readByteValue(is);
            case INT16:
            case UINT16:
                return (Object) readShortValue(is);
            case INT32:
            case UINT32:
                return (Object) readIntValue(is);
            case INT64:
            case UINT64:
                return (Object) readLongValue(is);
            case FLOAT:
                return (Object) readFloatValue(is);
            case DOUBLE:
                return (Object) readDoubleValue(is);
            case COMPLEX_DOUBLE:
                return (Object) readComplexDoubleValue(is);
            case BOOL:
                return (Object) readBooleanValue(is);
            case STRING:
                return (Object) readStringValue(is);
            case HASH:
                return (Object) readHashValue(is);
            case SCHEMA:
                return (Object) readSchemaValue(is);
            case NONE:
                return (Object) readNoneValue(is);
            default:
                throw new IOException("Encountered unknown data type whilst reading from binary archive");
        }
    }

    private VectorBoolean readSequenceBool(ByteBuffer is, int size) throws IOException {
        VectorBoolean vec = new VectorBoolean();
        for (int i = 0; i < size; i++) {
            boolean e = readBooleanValue(is);
            vec.add(e);
        }
        return vec;
    }

    private VectorString readSequenceString(ByteBuffer is, int size) throws IOException {
        VectorString vec = new VectorString();
        for (int i = 0; i < size; i++) {
            String e = readStringValue(is);
            vec.add(e);
        }
        return vec;
    }

    private VectorByte readSequenceByte(ByteBuffer is, int size) throws IOException {
        VectorByte vec = new VectorByte();
        for (int i = 0; i < size; i++) {
            byte e = readByteValue(is);
            vec.add(e);
        }
        return vec;
    }

    private VectorShort readSequenceShort(ByteBuffer is, int size) throws IOException {
        VectorShort vec = new VectorShort();
        for (int i = 0; i < size; i++) {
            short e = readShortValue(is);
            vec.add(e);
        }
        return vec;
    }

    private VectorInteger readSequenceInt(ByteBuffer is, int size) throws IOException {
        VectorInteger vec = new VectorInteger();
        for (int i = 0; i < size; i++) {
            int e = readIntValue(is);
            vec.add(e);
        }
        return vec;
    }

    private VectorLong readSequenceLong(ByteBuffer is, int size) throws IOException {
        VectorLong vec = new VectorLong();
        for (int i = 0; i < size; i++) {
            long e = readLongValue(is);
            vec.add(e);
        }
        return vec;
    }

    private VectorFloat readSequenceFloat(ByteBuffer is, int size) throws IOException {
        VectorFloat vec = new VectorFloat();
        for (int i = 0; i < size; i++) {
            float e = readFloatValue(is);
            vec.add(e);
        }
        return vec;
    }

    private VectorDouble readSequenceDouble(ByteBuffer is, int size) throws IOException {
        VectorDouble vec = new VectorDouble();
        for (int i = 0; i < size; i++) {
            double e = readDoubleValue(is);
            vec.add(e);
        }
        return vec;
    }

    private VectorComplexDouble readSequenceComplexDouble(ByteBuffer is, int size) throws IOException {
        VectorComplexDouble vec = new VectorComplexDouble();
        for (int i = 0; i < size; i++) {
            ComplexDouble e = readComplexDoubleValue(is);
            vec.add(e);
        }
        return vec;
    }

    private VectorHash readSequenceHash(ByteBuffer is, int size) throws IOException {
        VectorHash vec = new VectorHash();
        for (int i = 0; i < size; i++) {
            Hash e = readHashValue(is);
            vec.add(e);
        }
        return vec;
    }

    private VectorNone readSequenceNone(ByteBuffer is, int size) throws IOException {
        VectorNone vec = new VectorNone();
        for (int i = 0; i < size; i++) {
            CppNone e = readNoneValue(is);
            vec.add(e);
        }
        return vec;
    }

    private Object readSequence(ByteBuffer is, Types.ReferenceType type) throws IOException {
        int size = readSize(is);
        switch (type) {
            case VECTOR_BOOL:
                return readSequenceBool(is, size);
            case VECTOR_STRING:
                return readSequenceString(is, size);
            case VECTOR_CHAR:
            case VECTOR_INT8:
            case VECTOR_UINT8:
                return readSequenceByte(is, size);
            case VECTOR_INT16:
            case VECTOR_UINT16:
                return readSequenceShort(is, size);
            case VECTOR_INT32:
            case VECTOR_UINT32:
                return readSequenceInt(is, size);
            case VECTOR_INT64:
            case VECTOR_UINT64:
                return readSequenceLong(is, size);
            case VECTOR_FLOAT:
                return readSequenceFloat(is, size);
            case VECTOR_DOUBLE:
                return readSequenceDouble(is, size);
            case VECTOR_COMPLEX_DOUBLE:
                return readSequenceComplexDouble(is, size);
            case VECTOR_HASH:
                return readSequenceHash(is, size);
            case VECTOR_NONE:
                return readSequenceNone(is, size);
            default:
                throw new IOException("Encountered unknown array data type whilst reading from binary archive");
        }
    }
}
