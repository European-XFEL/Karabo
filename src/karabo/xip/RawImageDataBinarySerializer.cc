/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 * 
 * Created on September 5, 2013, 5:53 PM
 */

#include "RawImageDataBinarySerializer.hh"
#include "karabo/io/BinaryFileOutput.hh"
#include <karabo/xms/NetworkOutput.hh>
#include <karabo/xms/NetworkInput.hh>

namespace karabo {
    namespace xip {

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<RawImageData>, RawImageBinarySerializer)
                
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<RawImageData >, karabo::io::BinaryFileOutput<RawImageData>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<RawImageData >, karabo::io::BinaryFileInput<RawImageData>)
                
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<RawImageData >, karabo::xms::NetworkOutput<RawImageData >)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<RawImageData >, karabo::xms::NetworkInput<RawImageData >)
    }
}


