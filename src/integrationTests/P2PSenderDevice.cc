/* 
 * File:   P2PSenderDeviceDevice.cc
 * Author: haufs
 * 
 * Created on September 20, 2016, 3:49 PM
 */


#include "P2PSenderDevice.hh"

using namespace std;

USING_KARABO_NAMESPACES;

namespace karabo {

    
    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, P2PSenderDevice)

    void P2PSenderDevice::expectedParameters(Schema& expected) {

        
        Schema data;
        INT32_ELEMENT(data).key("dataId")
                .readOnly()
                .commit();

        STRING_ELEMENT(data).key("sha1")
                .readOnly()
                .commit();

        STRING_ELEMENT(data).key("flow")
                .readOnly()
                .commit();

        VECTOR_INT64_ELEMENT(data).key("data")
                .readOnly()
                .commit();
        
        NDARRAY_ELEMENT(data).key("array")
                .dtype(Types::DOUBLE)
                .shape("100,200,0")
                .commit();

        OUTPUT_CHANNEL(expected).key("output1")
                .displayedName("Output1")
                .dataSchema(data)
                .commit();

    
        
    }


    P2PSenderDevice::P2PSenderDevice(const Hash& config) : Device<>(config) {

    }

    P2PSenderDevice::~P2PSenderDevice() {
 
    };

}