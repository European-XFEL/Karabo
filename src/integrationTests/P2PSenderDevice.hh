/*
 * File:   P2PSenderDevice.hh
 * Author: haufs
 *
 * Created on September 20, 2016, 3:49 PM
 */

#ifndef P2PSENDERDEVICE_HH
#define	P2PSENDERDEVICE_HH

#include <karabo/karabo.hpp>

namespace karabo {

    class P2PSenderDevice : public karabo::core::Device<> {

    public:

        KARABO_CLASSINFO(P2PSenderDevice, "P2PSenderDevice", "2.0")

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        P2PSenderDevice(const karabo::util::Hash& input);

        virtual ~P2PSenderDevice();

    };
}


#endif	/* P2PSENDERDEVICE_HH */

