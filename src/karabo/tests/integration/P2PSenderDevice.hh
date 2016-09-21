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

        /// thread for actual work
        boost::thread m_writingThread;

        int m_currentDataId;

        bool m_isStopped;

    public:

        KARABO_CLASSINFO(P2PSenderDevice, "P2PSenderDevice", "2.0")

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        P2PSenderDevice(const karabo::util::Hash& input);

        virtual ~P2PSenderDevice();

    private:
        //slots for write and stop commands
        void write();

        void stop();

        /// method for writing thread
        void writing();
    };
}


#endif	/* P2PSENDERDEVICE_HH */

