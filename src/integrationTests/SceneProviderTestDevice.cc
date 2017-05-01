/* 
 * File:   SceneProviderTestDevice.cc
 * Author: steffen.hauf@xfel.eu
 * 
 */

#include "SceneProviderTestDevice.hh"

using namespace std;

USING_KARABO_NAMESPACES

        namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, SceneProviderTestDevice)

    void SceneProviderTestDevice::expectedParameters(Schema& expected) {

        VECTOR_STRING_ELEMENT(expected).key("availableScenes")
                .readOnly().initialValue(std::vector<string>())
                .commit();

    }


    SceneProviderTestDevice::SceneProviderTestDevice(const karabo::util::Hash& config) : Device<>(config) {
        KARABO_INITIAL_FUNCTION(initialize);


    }


    SceneProviderTestDevice::~SceneProviderTestDevice() {
    }


    void SceneProviderTestDevice::initialize() {
    }

}