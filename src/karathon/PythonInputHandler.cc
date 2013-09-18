#include "PythonInputHandler.hh"

namespace karathon {
    KARABO_REGISTER_IN_FACTORY(karabo::io::InputHandler, PythonInputHandler);
    
    KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler, PythonInputHandler, karabo::io::AbstractInput::Pointer);
}


