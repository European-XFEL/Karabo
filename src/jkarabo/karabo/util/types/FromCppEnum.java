/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.util.types;

import java.util.HashMap;
import java.util.Map;
import karabo.util.types.Types.ReferenceType;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
public class FromCppEnum {
    
    private final Map<Integer, ReferenceType> _typeInfoMap = new HashMap<>();
    
    private FromCppEnum() {
        _typeInfoMap.put(ReferenceType.BOOL.getValue(), ReferenceType.BOOL);
        _typeInfoMap.put(ReferenceType.VECTOR_BOOL.getValue(), ReferenceType.VECTOR_BOOL);
        _typeInfoMap.put(ReferenceType.CHAR.getValue(), ReferenceType.CHAR);
        _typeInfoMap.put(ReferenceType.VECTOR_CHAR.getValue(), ReferenceType.VECTOR_CHAR);
        _typeInfoMap.put(ReferenceType.INT8.getValue(), ReferenceType.INT8);
        _typeInfoMap.put(ReferenceType.VECTOR_INT8.getValue(), ReferenceType.VECTOR_INT8);
        _typeInfoMap.put(ReferenceType.UINT8.getValue(), ReferenceType.UINT8);
        _typeInfoMap.put(ReferenceType.VECTOR_UINT8.getValue(), ReferenceType.VECTOR_UINT8);
        _typeInfoMap.put(ReferenceType.INT16.getValue(), ReferenceType.INT16);
        _typeInfoMap.put(ReferenceType.VECTOR_INT16.getValue(), ReferenceType.VECTOR_INT16);
        _typeInfoMap.put(ReferenceType.UINT16.getValue(), ReferenceType.UINT16);
        _typeInfoMap.put(ReferenceType.VECTOR_UINT16.getValue(), ReferenceType.VECTOR_UINT16);
        _typeInfoMap.put(ReferenceType.INT32.getValue(), ReferenceType.INT32);
        _typeInfoMap.put(ReferenceType.VECTOR_INT32.getValue(), ReferenceType.VECTOR_INT32);
        _typeInfoMap.put(ReferenceType.UINT32.getValue(), ReferenceType.UINT32);
        _typeInfoMap.put(ReferenceType.VECTOR_UINT32.getValue(), ReferenceType.VECTOR_UINT32);
        _typeInfoMap.put(ReferenceType.INT64.getValue(), ReferenceType.INT64);
        _typeInfoMap.put(ReferenceType.VECTOR_INT64.getValue(), ReferenceType.VECTOR_INT64);
        _typeInfoMap.put(ReferenceType.UINT64.getValue(), ReferenceType.UINT64);
        _typeInfoMap.put(ReferenceType.VECTOR_UINT64.getValue(), ReferenceType.VECTOR_UINT64);
        _typeInfoMap.put(ReferenceType.FLOAT.getValue(), ReferenceType.FLOAT);
        _typeInfoMap.put(ReferenceType.VECTOR_FLOAT.getValue(), ReferenceType.VECTOR_FLOAT);
        _typeInfoMap.put(ReferenceType.DOUBLE.getValue(), ReferenceType.DOUBLE);
        _typeInfoMap.put(ReferenceType.VECTOR_DOUBLE.getValue(), ReferenceType.VECTOR_DOUBLE);
        _typeInfoMap.put(ReferenceType.COMPLEX_FLOAT.getValue(), ReferenceType.COMPLEX_FLOAT);
        _typeInfoMap.put(ReferenceType.VECTOR_COMPLEX_FLOAT.getValue(), ReferenceType.VECTOR_COMPLEX_FLOAT);
        _typeInfoMap.put(ReferenceType.COMPLEX_DOUBLE.getValue(), ReferenceType.COMPLEX_DOUBLE);
        _typeInfoMap.put(ReferenceType.VECTOR_COMPLEX_DOUBLE.getValue(), ReferenceType.VECTOR_COMPLEX_DOUBLE);
        _typeInfoMap.put(ReferenceType.STRING.getValue(), ReferenceType.STRING);
        _typeInfoMap.put(ReferenceType.VECTOR_STRING.getValue(), ReferenceType.VECTOR_STRING);
        _typeInfoMap.put(ReferenceType.HASH.getValue(), ReferenceType.HASH);
        _typeInfoMap.put(ReferenceType.VECTOR_HASH.getValue(), ReferenceType.VECTOR_HASH);
        _typeInfoMap.put(ReferenceType.PTR_BOOL.getValue(), ReferenceType.PTR_BOOL);
        _typeInfoMap.put(ReferenceType.PTR_CHAR.getValue(), ReferenceType.PTR_CHAR);
        _typeInfoMap.put(ReferenceType.PTR_INT8.getValue(), ReferenceType.PTR_INT8);
        _typeInfoMap.put(ReferenceType.PTR_UINT8.getValue(), ReferenceType.PTR_UINT8);
        _typeInfoMap.put(ReferenceType.PTR_INT16.getValue(), ReferenceType.PTR_INT16);
        _typeInfoMap.put(ReferenceType.PTR_UINT16.getValue(), ReferenceType.PTR_UINT16);
        _typeInfoMap.put(ReferenceType.PTR_INT32.getValue(), ReferenceType.PTR_INT32);
        _typeInfoMap.put(ReferenceType.PTR_UINT32.getValue(), ReferenceType.PTR_UINT32);
        _typeInfoMap.put(ReferenceType.PTR_INT64.getValue(), ReferenceType.PTR_INT64);
        _typeInfoMap.put(ReferenceType.PTR_UINT64.getValue(), ReferenceType.PTR_UINT64);
        _typeInfoMap.put(ReferenceType.PTR_FLOAT.getValue(), ReferenceType.PTR_FLOAT);
        _typeInfoMap.put(ReferenceType.PTR_DOUBLE.getValue(), ReferenceType.PTR_DOUBLE);
        _typeInfoMap.put(ReferenceType.PTR_COMPLEX_FLOAT.getValue(), ReferenceType.PTR_COMPLEX_FLOAT);
        _typeInfoMap.put(ReferenceType.PTR_COMPLEX_DOUBLE.getValue(), ReferenceType.PTR_COMPLEX_DOUBLE);
        _typeInfoMap.put(ReferenceType.PTR_STRING.getValue(), ReferenceType.PTR_STRING);
        _typeInfoMap.put(ReferenceType.SCHEMA.getValue(), ReferenceType.SCHEMA);
        _typeInfoMap.put(ReferenceType.VECTOR_SCHEMA.getValue(), ReferenceType.VECTOR_SCHEMA);
        _typeInfoMap.put(ReferenceType.ANY.getValue(), ReferenceType.ANY);
        _typeInfoMap.put(ReferenceType.NONE.getValue(), ReferenceType.NONE);
        _typeInfoMap.put(ReferenceType.VECTOR_NONE.getValue(), ReferenceType.VECTOR_NONE);
        _typeInfoMap.put(ReferenceType.UNKNOWN.getValue(), ReferenceType.UNKNOWN);
        _typeInfoMap.put(ReferenceType.SIMPLE.getValue(), ReferenceType.SIMPLE);
        _typeInfoMap.put(ReferenceType.SEQUENCE.getValue(), ReferenceType.SEQUENCE);
        _typeInfoMap.put(ReferenceType.POINTER.getValue(), ReferenceType.POINTER);
        
    }
    
    private static FromCppEnum singleInstance = new FromCppEnum();
    
    public static ReferenceType from(int cppType) throws InterruptedException {
//        System.out.println("CPPtype: " + cppType);
        if (!singleInstance._typeInfoMap.containsKey(cppType)) {
            throw new RuntimeException("Requested argument type not registered: " + cppType);
        }
        return singleInstance._typeInfoMap.get(cppType);
    }
}
