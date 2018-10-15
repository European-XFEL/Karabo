/*
 * File:   P2PSenderDevice.hh
 * Author: haufs
 *
 * Created on September 20, 2016, 3:49 PM
 */

#ifndef P2P_SENDER_DEVICE_HH
#define P2P_SENDER_DEVICE_HH

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

    private:

        // slot for write command
        void write();
        // slot for stopping the writing activity
        void stopWrite();
        // slot for stopping the writingProfile activity
        void stopWriteProfile();

        /// method for writing thread
        void writing();
        void writingProfile();

        boost::thread m_writingThread;

        bool m_stopWriting;
        bool m_stopWritingProfile;

    };
}


#endif /* P2P_SENDER_DEVICE_HH */

