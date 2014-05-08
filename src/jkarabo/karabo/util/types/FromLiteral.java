/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util.types;

import java.util.HashMap;
import java.util.Map;
import karabo.util.types.Types.ReferenceType;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class FromLiteral {
    
    private final Map<String, ReferenceType> _typeInfoMap = new HashMap<>();
    
    private FromLiteral() {
        _typeInfoMap.put("BOOL", ReferenceType.BOOL);
        _typeInfoMap.put("VECTOR_BOOL", ReferenceType.VECTOR_BOOL);
        _typeInfoMap.put("CHAR", ReferenceType.CHAR);
        _typeInfoMap.put("VECTOR_CHAR", ReferenceType.VECTOR_CHAR);
        _typeInfoMap.put("INT8", ReferenceType.INT8);
        _typeInfoMap.put("VECTOR_INT8", ReferenceType.VECTOR_INT8);
        _typeInfoMap.put("UINT8", ReferenceType.UINT8);
        _typeInfoMap.put("VECTOR_UINT8", ReferenceType.VECTOR_UINT8);
        _typeInfoMap.put("INT16", ReferenceType.INT16);
        _typeInfoMap.put("VECTOR_INT16", ReferenceType.VECTOR_INT16);
        _typeInfoMap.put("UINT16", ReferenceType.UINT16);
        _typeInfoMap.put("VECTOR_UINT16", ReferenceType.VECTOR_UINT16);
        _typeInfoMap.put("INT32", ReferenceType.INT32);
        _typeInfoMap.put("VECTOR_INT32", ReferenceType.VECTOR_INT32);
        _typeInfoMap.put("UINT32", ReferenceType.UINT32);
        _typeInfoMap.put("VECTOR_UINT32", ReferenceType.VECTOR_UINT32);
        _typeInfoMap.put("INT64", ReferenceType.INT64);
        _typeInfoMap.put("VECTOR_INT64", ReferenceType.VECTOR_INT64);
        _typeInfoMap.put("UINT64", ReferenceType.UINT64);
        _typeInfoMap.put("VECTOR_UINT64", ReferenceType.VECTOR_UINT64);
        _typeInfoMap.put("FLOAT", ReferenceType.FLOAT);
        _typeInfoMap.put("VECTOR_FLOAT", ReferenceType.VECTOR_FLOAT);
        _typeInfoMap.put("DOUBLE", ReferenceType.DOUBLE);
        _typeInfoMap.put("VECTOR_DOUBLE", ReferenceType.VECTOR_DOUBLE);
        _typeInfoMap.put("STRING", ReferenceType.STRING);
        _typeInfoMap.put("VECTOR_STRING", ReferenceType.VECTOR_STRING);
        _typeInfoMap.put("HASH", ReferenceType.HASH);
        _typeInfoMap.put("VECTOR_HASH", ReferenceType.VECTOR_HASH);
        _typeInfoMap.put("SCHEMA", ReferenceType.SCHEMA);
        _typeInfoMap.put("COMPLEX_FLOAT", ReferenceType.COMPLEX_FLOAT);
        _typeInfoMap.put("VECTOR_COMPLEX_FLOAT", ReferenceType.VECTOR_COMPLEX_FLOAT);
        _typeInfoMap.put("COMPLEX_DOUBLE", ReferenceType.COMPLEX_DOUBLE);
        _typeInfoMap.put("VECTOR_COMPLEX_DOUBLE", ReferenceType.VECTOR_COMPLEX_DOUBLE);
        _typeInfoMap.put("PTR_BOOL", ReferenceType.PTR_BOOL);
        _typeInfoMap.put("PTR_CHAR", ReferenceType.PTR_CHAR);
        _typeInfoMap.put("PTR_INT8", ReferenceType.PTR_INT8);
        _typeInfoMap.put("PTR_UINT8", ReferenceType.PTR_UINT8);
        _typeInfoMap.put("PTR_INT16", ReferenceType.PTR_INT16);
        _typeInfoMap.put("PTR_UINT16", ReferenceType.PTR_UINT16);
        _typeInfoMap.put("PTR_INT32", ReferenceType.PTR_INT32);
        _typeInfoMap.put("PTR_UINT32", ReferenceType.PTR_UINT32);
        _typeInfoMap.put("PTR_INT64", ReferenceType.PTR_INT64);
        _typeInfoMap.put("PTR_UINT64", ReferenceType.PTR_UINT64);
        _typeInfoMap.put("PTR_FLOAT", ReferenceType.PTR_FLOAT);
        _typeInfoMap.put("PTR_DOUBLE", ReferenceType.PTR_DOUBLE);
        _typeInfoMap.put("PTR_COMPLEX_FLOAT", ReferenceType.PTR_COMPLEX_FLOAT);
        _typeInfoMap.put("PTR_COMPLEX_DOUBLE", ReferenceType.PTR_COMPLEX_DOUBLE);
        _typeInfoMap.put("PTR_STRING", ReferenceType.PTR_STRING);
        _typeInfoMap.put("NONE", ReferenceType.NONE);
        _typeInfoMap.put("VECTOR_NONE", ReferenceType.VECTOR_NONE);
    }
    
    private static FromLiteral singleInstance = new FromLiteral();

    public static ReferenceType from(String type) {
        if (!singleInstance._typeInfoMap.containsKey(type)) {
            throw new RuntimeException("Requested argument type not registered");
        }
        return singleInstance._typeInfoMap.get(type);
    }
}
