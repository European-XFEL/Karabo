#include "CppInputHandler.hh"

namespace karabo {
    namespace io {
        KARABO_REGISTER_IN_FACTORY(InputHandler, CppInputHandler);
        
        KARABO_REGISTER_IN_FACTORY_1(InputHandler, CppInputHandler, AbstractInput::Pointer);
    }
}
