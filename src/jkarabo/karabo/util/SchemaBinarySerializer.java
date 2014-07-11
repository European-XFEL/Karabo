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

/**
 *
 * @author esenov
 */
@KARABO_CLASSINFO(classId = "Bin", version = "1.0")
public class SchemaBinarySerializer extends BinarySerializerSchema {

    private BinarySerializerHash m_serializer;
    private static final ByteOrder byteOrder = ByteOrder.LITTLE_ENDIAN;
    
    static {
        KARABO_REGISTER_FOR_CONFIGURATION(BinarySerializerSchema.class, SchemaBinarySerializer.class);
        //System.out.println("SchemaBinarySerializer class static registration");
    }
    
    SchemaBinarySerializer(Hash input) {
        m_serializer = BinarySerializerHash.create(BinarySerializerHash.class, "Bin", input);
    }

    public static void expectedParameters(Schema expected) {
    }
    
    @Override
    public ByteBuffer save(Schema object) throws IOException {
        ByteArrayOutputStream os = new ByteArrayOutputStream(4096);
        writeString(os, object.getRootName());
        writeBuffer(os, m_serializer.save(object.getParameterHash()));
        byte b[] = os.toByteArray();
        return ByteBuffer.wrap(b);
    }

    @Override
    public Schema load(byte[] archive, int offset, int nBytes) throws IOException {
        ByteBuffer is = ByteBuffer.wrap(archive, offset, nBytes);
        is.order(byteOrder);
        String root = readString(is);
        Schema schema = new Schema();
        schema.setRootName(root);
        Hash hash = m_serializer.load(is);
        schema.setParameterHash(hash);
        schema.updateAliasMap();
        return schema;
    }
    
    private void writeString(OutputStream os, String value) throws IOException {
        ByteBuffer bb = ByteBuffer.allocate(Byte.SIZE/8);
        //bb.order(byteOrder);
        int len = value.length();
        bb.put((byte)len);
        //System.out.println("*** writeString: value.length=" + len + " value=" + value);
        os.write(bb.array());
        os.write(value.getBytes());
    }

    private String readString(ByteBuffer is) throws IOException {
        try {
            int size = (int)is.get();
            //System.out.println("*** readString: size = " + size);
            byte[] dst = new byte[size];
            is.get(dst);
            String result = new String(dst);
            //System.out.println("*** readString: size = " + size + " value=" + result);
            return result;
        } catch(BufferUnderflowException ex) {
            throw new IOException("Premature EOF");
        }
    }
    private void writeBuffer(OutputStream os, ByteBuffer bb) throws IOException {
        os.write(bb.array());
    }
}
