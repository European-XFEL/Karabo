#include "CppOutputHandler.hh"

namespace karabo {
    namespace io {
        KARABO_REGISTER_IN_FACTORY(OutputHandler, CppOutputHandler);
        KARABO_REGISTER_IN_FACTORY_1(OutputHandler, CppOutputHandler, AbstractOutput::Pointer);
    }
}

