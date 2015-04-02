/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 * 
 * Created on September 5, 2013, 5:53 PM
 */

#include "RawImageDataBinarySerializer.hh"
#include "karabo/io/BinaryFileOutput.hh"
#include <karabo/io/CppInputHandler.hh>
//#include <karabo/xms/NetworkOutput.hh>
#include <karabo/xms/NetworkInput.hh>

namespace karabo {
    namespace xip {

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<RawImageData>, RawImageBinarySerializer)
                
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<RawImageData >, karabo::io::BinaryFileOutput<RawImageData>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<RawImageData >, karabo::io::BinaryFileInput<RawImageData>)
                
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<RawImageData >, karabo::xms::NetworkInput<RawImageData >)

        KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler, karabo::io::CppInputHandler<karabo::io::Input<RawImageData> > , karabo::io::AbstractInput::Pointer);

    }
}


