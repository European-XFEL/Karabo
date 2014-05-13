/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.types;

import java.util.HashMap;
import java.util.Map;
import karabo.util.CppNone;
import karabo.util.Hash;
import karabo.util.Schema;
import karabo.util.types.Types;
import karabo.util.vectors.VectorBoolean;
import karabo.util.vectors.VectorByte;
import karabo.util.vectors.VectorCharacter;
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
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class FromType<ValueType> {

    private Map<Class<?>, Types.ReferenceType> m_typeinfo = new HashMap<>();

    private FromType() {
        m_typeinfo.put(boolean.class, Types.ReferenceType.BOOL);
        m_typeinfo.put(Boolean.class, Types.ReferenceType.BOOL);
        m_typeinfo.put(VectorBoolean.class, Types.ReferenceType.VECTOR_BOOL);
        m_typeinfo.put(char.class, Types.ReferenceType.CHAR);
        m_typeinfo.put(Character.class, Types.ReferenceType.CHAR);
        m_typeinfo.put(VectorCharacter.class, Types.ReferenceType.VECTOR_CHAR);
        m_typeinfo.put(byte.class, Types.ReferenceType.INT8);
        m_typeinfo.put(Byte.class, Types.ReferenceType.INT8);
        m_typeinfo.put(VectorByte.class, Types.ReferenceType.VECTOR_INT8);
        m_typeinfo.put(short.class, Types.ReferenceType.INT16);
        m_typeinfo.put(Short.class, Types.ReferenceType.INT16);
        m_typeinfo.put(VectorShort.class, Types.ReferenceType.VECTOR_INT16);
        m_typeinfo.put(int.class, Types.ReferenceType.INT32);
        m_typeinfo.put(Integer.class, Types.ReferenceType.INT32);
        m_typeinfo.put(VectorInteger.class, Types.ReferenceType.VECTOR_INT32);
        m_typeinfo.put(long.class, Types.ReferenceType.INT64);
        m_typeinfo.put(Long.class, Types.ReferenceType.INT64);
        m_typeinfo.put(VectorLong.class, Types.ReferenceType.VECTOR_INT64);
        m_typeinfo.put(float.class, Types.ReferenceType.FLOAT);
        m_typeinfo.put(Float.class, Types.ReferenceType.FLOAT);
        m_typeinfo.put(VectorFloat.class, Types.ReferenceType.VECTOR_FLOAT);
        m_typeinfo.put(double.class, Types.ReferenceType.DOUBLE);
        m_typeinfo.put(Double.class, Types.ReferenceType.DOUBLE);
        m_typeinfo.put(VectorDouble.class, Types.ReferenceType.VECTOR_DOUBLE);
        m_typeinfo.put(ComplexDouble.class, Types.ReferenceType.COMPLEX_DOUBLE);
        m_typeinfo.put(VectorComplexDouble.class, Types.ReferenceType.VECTOR_COMPLEX_DOUBLE);
        m_typeinfo.put(String.class, Types.ReferenceType.STRING);
        m_typeinfo.put(VectorString.class, Types.ReferenceType.VECTOR_STRING);
        m_typeinfo.put(Hash.class, Types.ReferenceType.HASH);
        m_typeinfo.put(VectorHash.class, Types.ReferenceType.VECTOR_HASH);
        m_typeinfo.put(Schema.class, Types.ReferenceType.SCHEMA);
        m_typeinfo.put(CppNone.class, Types.ReferenceType.NONE);
        m_typeinfo.put(VectorNone.class, Types.ReferenceType.VECTOR_NONE);
    }

    public static boolean isPrimitive(Class<?> clazz) {
        if (clazz == byte.class
                || clazz == boolean.class
                || clazz == char.class
                || clazz == short.class
                || clazz == int.class
                || clazz == long.class
                || clazz == float.class
                || clazz == double.class
                || clazz == void.class) {
            return true;
        }
        return false;
    }

    public static boolean isWrapper(Class<?> clazz) {
        if (clazz == Byte.class
                || clazz == Boolean.class
                || clazz == Character.class
                || clazz == Short.class
                || clazz == Integer.class
                || clazz == Long.class
                || clazz == Float.class
                || clazz == Double.class
                || clazz == ComplexDouble.class
                || clazz == String.class
                || clazz == Void.class
                || clazz == Hash.class
                || clazz == Schema.class
                || clazz == CppNone.class
                || clazz == VectorByte.class
                || clazz == VectorBoolean.class
                || clazz == VectorCharacter.class
                || clazz == VectorShort.class
                || clazz == VectorInteger.class
                || clazz == VectorLong.class
                || clazz == VectorFloat.class
                || clazz == VectorDouble.class
                || clazz == VectorComplexDouble.class
                || clazz == VectorString.class
                || clazz == VectorHash.class
                || clazz == VectorNone.class) {
            return true;
        }
        return false;
    }

    private static FromType singleInstance = new FromType();

    public static Types.ReferenceType from(Class<?> type) {
        if (singleInstance.isWrapper(type)) {
            return (Types.ReferenceType) singleInstance.m_typeinfo.get(type);
        }
        throw new RuntimeException("This type \"" + type.toString() + "\" not supported.");
    }

    public static Types.ReferenceType fromInstance(Object object) {
        if (object instanceof Boolean) {
            return Types.ReferenceType.BOOL;
        } else if (object instanceof VectorBoolean) {
            return Types.ReferenceType.VECTOR_BOOL;
        } else if (object instanceof Character) {
            return Types.ReferenceType.CHAR;
        } else if (object instanceof VectorCharacter) {
            return Types.ReferenceType.VECTOR_CHAR;
        } else if (object instanceof Byte) {
            return Types.ReferenceType.INT8;
        } else if (object instanceof VectorByte) {
            return Types.ReferenceType.VECTOR_INT8;
        } else if (object instanceof Short) {
            return Types.ReferenceType.INT16;
        } else if (object instanceof VectorShort) {
            return Types.ReferenceType.VECTOR_INT16;
        } else if (object instanceof Integer) {
            return Types.ReferenceType.INT32;
        } else if (object instanceof VectorInteger) {
            return Types.ReferenceType.VECTOR_INT32;
        } else if (object instanceof Long) {
            return Types.ReferenceType.INT64;
        } else if (object instanceof VectorLong) {
            return Types.ReferenceType.VECTOR_INT64;
        } else if (object instanceof Float) {
            return Types.ReferenceType.FLOAT;
        } else if (object instanceof VectorFloat) {
            return Types.ReferenceType.VECTOR_FLOAT;
        } else if (object instanceof Double) {
            return Types.ReferenceType.DOUBLE;
        } else if (object instanceof VectorDouble) {
            return Types.ReferenceType.VECTOR_DOUBLE;
        } else if (object instanceof ComplexDouble) {
            return Types.ReferenceType.COMPLEX_DOUBLE;
        } else if (object instanceof VectorComplexDouble) {
            return Types.ReferenceType.VECTOR_COMPLEX_DOUBLE;
        } else if (object instanceof String) {
            return Types.ReferenceType.STRING;
        } else if (object instanceof VectorString) {
            return Types.ReferenceType.VECTOR_STRING;
        } else if (object instanceof Hash) {
            return Types.ReferenceType.HASH;
        } else if (object instanceof VectorHash) {
            return Types.ReferenceType.VECTOR_HASH;
        } else if (object instanceof Schema) {
            return Types.ReferenceType.SCHEMA;
        } else if (object instanceof CppNone) {
            return Types.ReferenceType.NONE;
        } else if (object instanceof VectorNone) {
            return Types.ReferenceType.VECTOR_NONE;
        } else {
            return Types.ReferenceType.UNKNOWN;
        }
    }

    public static String toString(Object object) {
        if (object instanceof Boolean) {
            return ((Boolean) object).toString();
        } else if (object instanceof VectorBoolean) {
            return ((VectorBoolean) object).toString();
        } else if (object instanceof Character) {
            return ((Character) object).toString();
        } else if (object instanceof VectorCharacter) {
            return ((VectorCharacter) object).toString();
        } else if (object instanceof Byte) {
            return ((Byte) object).toString();
        } else if (object instanceof VectorByte) {
            return ((VectorByte) object).toString();
        } else if (object instanceof Short) {
            return ((Short) object).toString();
        } else if (object instanceof VectorShort) {
            return ((VectorShort) object).toString();
        } else if (object instanceof Integer) {
            return ((Integer) object).toString();
        } else if (object instanceof VectorInteger) {
            return ((VectorInteger) object).toString();
        } else if (object instanceof Long) {
            return ((Long) object).toString();
        } else if (object instanceof VectorLong) {
            return ((VectorLong) object).toString();
        } else if (object instanceof Float) {
            return ((Float) object).toString();
        } else if (object instanceof VectorFloat) {
            return ((VectorFloat) object).toString();
        } else if (object instanceof Double) {
            return ((Double) object).toString();
        } else if (object instanceof VectorDouble) {
            return ((VectorDouble) object).toString();
        } else if (object instanceof ComplexDouble) {
            return ((ComplexDouble)object).toString();
        } else if (object instanceof VectorComplexDouble) {
            return ((VectorComplexDouble)object).toString();
        } else if (object instanceof String) {
            return (String)object;
        } else if (object instanceof VectorString) {
            return ((VectorString)object).toString();
        } else if (object instanceof Hash) {
            return ((Hash)object).toString();
        } else if (object instanceof VectorHash) {
            return ((VectorHash)object).toString();
        } else if (object instanceof Schema) {
            return ((Schema)object).toString();
        } else if (object instanceof CppNone) {
            return ((CppNone)object).toString();
        } else if (object instanceof VectorNone) {
            return ((VectorNone)object).toString();
        }
        throw new RuntimeException("Cannot convert to string the object with UNKNOWN reference type");
    }

}
