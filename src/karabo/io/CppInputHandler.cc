#include "CppInputHandler.hh"
#include <karabo/io/Input.hh>

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace io {

        KARABO_REGISTER_IN_FACTORY_1(InputHandler, CppInputHandler<Input<Hash> >, AbstractInput::Pointer);
        KARABO_REGISTER_IN_FACTORY_1(InputHandler, CppInputHandler<Input<std::vector<char> > >, AbstractInput::Pointer);
        // TODO Image !!

    }
}
