/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 10, 2012, 6:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/xms/NetworkInput.hh>
//#include <karabo/xms/NetworkOutput.hh>
#include <karabo/io/CppInputHandler.hh>

#include "CpuImageBinarySerializer.hh"

KARABO_EXPLICIT_TEMPLATE(karabo::io::BinarySerializer<karabo::xip::CpuImage<double> >)
KARABO_EXPLICIT_TEMPLATE(karabo::io::BinarySerializer<karabo::xip::CpuImage<float> >)
KARABO_EXPLICIT_TEMPLATE(karabo::io::BinarySerializer<karabo::xip::CpuImage<unsigned int> >)
KARABO_EXPLICIT_TEMPLATE(karabo::io::BinarySerializer<karabo::xip::CpuImage<unsigned short> >)
KARABO_EXPLICIT_TEMPLATE(karabo::io::BinarySerializer<karabo::xip::CpuImage<unsigned char> >)

namespace karabo {
    namespace xip {

        // Register into binary serializer factory
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<CpuImage<double> >, CpuImageBinarySerializer<double>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<CpuImage<float> >, CpuImageBinarySerializer<float>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<CpuImage<unsigned int> >, CpuImageBinarySerializer<unsigned int>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<CpuImage<unsigned short> >, CpuImageBinarySerializer<unsigned short>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<CpuImage<unsigned char> >, CpuImageBinarySerializer<unsigned char>)                        
                
        // Register into input factory
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::xip::CpuImage<double> >, karabo::xms::NetworkInput<karabo::xip::CpuImage<double> >)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::xip::CpuImage<float> >, karabo::xms::NetworkInput<karabo::xip::CpuImage<float> >)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::xip::CpuImage<unsigned int> >, karabo::xms::NetworkInput<karabo::xip::CpuImage<unsigned int> >)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::xip::CpuImage<unsigned short> >, karabo::xms::NetworkInput<karabo::xip::CpuImage<unsigned short> >)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::xip::CpuImage<unsigned char> >, karabo::xms::NetworkInput<karabo::xip::CpuImage<unsigned char> >)

        KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler, karabo::io::CppInputHandler<karabo::io::Input<karabo::xip::CpuImage<double> > > , karabo::io::AbstractInput::Pointer);
        KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler, karabo::io::CppInputHandler<karabo::io::Input<karabo::xip::CpuImage<float> > > , karabo::io::AbstractInput::Pointer);
        KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler, karabo::io::CppInputHandler<karabo::io::Input<karabo::xip::CpuImage<unsigned int> > > , karabo::io::AbstractInput::Pointer);
        KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler, karabo::io::CppInputHandler<karabo::io::Input<karabo::xip::CpuImage<unsigned short> > > , karabo::io::AbstractInput::Pointer);
        KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler, karabo::io::CppInputHandler<karabo::io::Input<karabo::xip::CpuImage<unsigned char> > > , karabo::io::AbstractInput::Pointer);
    }
}
