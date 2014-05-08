/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.types;

import karabo.util.types.Types.ReferenceType;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class ToLiteral {

    public static String to(ReferenceType type) {
        switch (type) {
            case BOOL:
                return "BOOL";
            case VECTOR_BOOL:
                return "VECTOR_BOOL";
            case CHAR:
                return "CHAR";
            case VECTOR_CHAR:
                return "VECTOR_CHAR";
            case INT8:
                return "INT8";
            case VECTOR_INT8:
                return "VECTOR_INT8";
            case UINT8:
                return "UINT8";
            case VECTOR_UINT8:
                return "VECTOR_UINT8";
            case INT16:
                return "INT16";
            case VECTOR_INT16:
                return "VECTOR_INT16";
            case UINT16:
                return "UINT16";
            case VECTOR_UINT16:
                return "VECTOR_UINT16";
            case INT32:
                return "INT32";
            case VECTOR_INT32:
                return "VECTOR_INT32";
            case UINT32:
                return "UINT32";
            case VECTOR_UINT32:
                return "VECTOR_UINT32";
            case INT64:
                return "INT64";
            case VECTOR_INT64:
                return "VECTOR_INT64";
            case UINT64:
                return "UINT64";
            case VECTOR_UINT64:
                return "VECTOR_UINT64";
            case FLOAT:
                return "FLOAT";
            case VECTOR_FLOAT:
                return "VECTOR_FLOAT";
            case DOUBLE:
                return "DOUBLE";
            case VECTOR_DOUBLE:
                return "VECTOR_DOUBLE";
            case STRING:
                return "STRING";
            case VECTOR_STRING:
                return "VECTOR_STRING";
            case HASH:
                return "HASH";
            case VECTOR_HASH:
                return "VECTOR_HASH";
            case SCHEMA:
                return "SCHEMA";
            case COMPLEX_FLOAT:
                return "COMPLEX_FLOAT";
            case VECTOR_COMPLEX_FLOAT:
                return "VECTOR_COMPLEX_FLOAT";
            case COMPLEX_DOUBLE:
                return "COMPLEX_DOUBLE";
            case VECTOR_COMPLEX_DOUBLE:
                return "VECTOR_COMPLEX_DOUBLE";
            case UNKNOWN:
                return "UNKNOWN";
            case PTR_BOOL:
                return "PTR_BOOL";
            case PTR_CHAR:
                return "PTR_CHAR";
            case PTR_INT8:
                return "PTR_INT8";
            case PTR_UINT8:
                return "PTR_UINT8";
            case PTR_INT16:
                return "PTR_INT16";
            case PTR_UINT16:
                return "PTR_UINT16";
            case PTR_INT32:
                return "PTR_INT32";
            case PTR_UINT32:
                return "PTR_UINT32";
            case PTR_INT64:
                return "PTR_INT64";
            case PTR_UINT64:
                return "PTR_UINT64";
            case PTR_FLOAT:
                return "PTR_FLOAT";
            case PTR_DOUBLE:
                return "PTR_DOUBLE";
            case PTR_COMPLEX_FLOAT:
                return "PTR_COMPLEX_FLOAT";
            case PTR_COMPLEX_DOUBLE:
                return "PTR_COMPLEX_DOUBLE";
            case PTR_STRING:
                return "PTR_STRING";
            case NONE:
                return "NONE";
            case VECTOR_NONE:
                return "VECTOR_NONE";
                
            default:
                throw new RuntimeException("Conversion to required type not implemented");
        }
    }
}
