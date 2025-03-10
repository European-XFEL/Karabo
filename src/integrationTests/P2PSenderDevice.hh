/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
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

    class P2PSenderDevice : public karabo::core::Device {
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
        void initialize();

        void preReconfigure(karabo::util::Hash& incomingReconfiguration) override;

        std::string selectSharedInput(const std::string& result,
                                      const std::vector<std::string>& connectedSharedInputs) const;

        // slot for write command
        void write();
        // slot for stopping the writing activity
        void stop();

        /// method for writing thread
        void writing(unsigned int dataSize);
        void writingProfile();

        std::jthread m_writingThread;

        bool m_stopWriting;
    };
} // namespace karabo


#endif /* P2P_SENDER_DEVICE_HH */
