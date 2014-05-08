/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.types;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Types {

    public enum ReferenceType {

        BOOL(0), // bool
        VECTOR_BOOL(1), // std::vector<std::bool>

        CHAR(2), // char
        VECTOR_CHAR(3), // std::vector<char>
        INT8(4), // signed char
        VECTOR_INT8(5), // std::vector<std::signed char>
        UINT8(6), // unsigned char
        VECTOR_UINT8(7), // std::vector<std::unsigned char>

        INT16(8), // signed short
        VECTOR_INT16(9), // std::vector<std::signed short>
        UINT16(10), // unsigned short
        VECTOR_UINT16(11), // std::vector<std::unsigned short>

        INT32(12), // signed int
        VECTOR_INT32(13), // std::vector<std::int>
        UINT32(14), // unsigned int
        VECTOR_UINT32(15), // std::vector<std::unsigned int>

        INT64(16), // signed long long
        VECTOR_INT64(17), // std::vector<std::signed long long>
        UINT64(18), // unsigned long long
        VECTOR_UINT64(19), // std::vector<std::unsigned long long>

        FLOAT(20), // float
        VECTOR_FLOAT(21), // std::vector<std::float>

        DOUBLE(22), // double
        VECTOR_DOUBLE(23), // std::vector<std::double>

        COMPLEX_FLOAT(24), // std::complex<float>
        VECTOR_COMPLEX_FLOAT(25), // std::vector<std::complex<float>

        COMPLEX_DOUBLE(26), // std::complex<double>
        VECTOR_COMPLEX_DOUBLE(27), // std::vector<std::complex<double>

        STRING(28), // std::string
        VECTOR_STRING(29), // std::vector<std::string>

        HASH(30), // Hash
        VECTOR_HASH(31), // std::vector<Hash>

        PTR_BOOL(32),
        PTR_CHAR(33),
        PTR_INT8(34),
        PTR_UINT8(35),
        PTR_INT16(36),
        PTR_UINT16(37),
        PTR_INT32(38),
        PTR_UINT32(39),
        PTR_INT64(40),
        PTR_UINT64(41),
        PTR_FLOAT(42),
        PTR_DOUBLE(43),
        PTR_COMPLEX_FLOAT(44),
        PTR_COMPLEX_DOUBLE(45),
        PTR_STRING(46),
        SCHEMA(47), // Schema
        VECTOR_SCHEMA(48), // std::vector<Schema>

        ANY(49), // unspecified type
        NONE(50), // CppNone type used during serialization/de-serialization
        VECTOR_NONE(51),

        UNKNOWN(52), // unknown type
        SIMPLE(53),
        SEQUENCE(54),
        POINTER(55);
        
        private final int id;
        ReferenceType(int id) {
            this.id = id;
        }
        public int getValue() {
            return id;
        }
    }

//    public <T extends Object> T to(Class<? extends ToType> cls, ReferenceType type) {
//        return (T)cls.getClass().to(type);
//    }
    public static ReferenceType category(Types.ReferenceType type) {
        switch (type) {
            case CHAR:
            case INT8:
            case INT16:
            case INT32:
            case INT64:
            case UINT8:
            case UINT16:
            case UINT32:
            case UINT64:
            case FLOAT:
            case DOUBLE:
            case BOOL:
            case STRING:
            case COMPLEX_FLOAT:
            case COMPLEX_DOUBLE:
            case NONE:
                return Types.ReferenceType.SIMPLE;
            case VECTOR_STRING:
            case VECTOR_CHAR:
            case VECTOR_INT8:
            case VECTOR_INT16:
            case VECTOR_INT32:
            case VECTOR_INT64:
            case VECTOR_UINT8:
            case VECTOR_UINT16:
            case VECTOR_UINT32:
            case VECTOR_UINT64:
            case VECTOR_DOUBLE:
            case VECTOR_FLOAT:
            case VECTOR_BOOL:
            case VECTOR_COMPLEX_FLOAT:
            case VECTOR_COMPLEX_DOUBLE:
            case VECTOR_NONE:
            case PTR_STRING:
            case PTR_CHAR:
            case PTR_INT8:
            case PTR_INT16:
            case PTR_INT32:
            case PTR_INT64:
            case PTR_UINT8:
            case PTR_UINT16:
            case PTR_UINT32:
            case PTR_UINT64:
            case PTR_DOUBLE:
            case PTR_FLOAT:
            case PTR_BOOL:
            case PTR_COMPLEX_FLOAT:
            case PTR_COMPLEX_DOUBLE:
                return Types.ReferenceType.SEQUENCE;
            case VECTOR_HASH:
                return Types.ReferenceType.VECTOR_HASH;
            case HASH:
                return Types.ReferenceType.HASH;
            case SCHEMA:
                return Types.ReferenceType.SCHEMA;
            case ANY:
                return Types.ReferenceType.ANY;
            default:
                return Types.ReferenceType.UNKNOWN;
        }
    }
}
