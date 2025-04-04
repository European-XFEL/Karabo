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
 * File:   PipeReceiverDevice.hh
 * Author: flucke
 *
 * Created on October 14, 2016, 2:30 PM
 */

#ifndef PIPE_RECEIVER_DEVICE_HH
#define PIPE_RECEIVER_DEVICE_HH

#include "karabo/core/Device.hh"

namespace karabo {

    namespace util {
        class Schema;
        class Hash;
    } // namespace util
    namespace xms {
        class InputChannel;
    }

    class PipeReceiverDevice : public karabo::core::Device {
       public:
        KARABO_CLASSINFO(PipeReceiverDevice, "PipeReceiverDevice", "2.0")

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::data::Schema& expected);

        PipeReceiverDevice(const karabo::data::Hash& config);
        virtual ~PipeReceiverDevice() {}

       private:
        void initialization();

        void onInput(const karabo::xms::InputChannel::Pointer& input);

        void onData(const karabo::data::Hash& data, const xms::InputChannel::MetaData& metaData);
        void onInputProfile(const xms::InputChannel::Pointer& input);
        void reset();

        void onEndOfStream(const karabo::xms::InputChannel::Pointer& input);
        void onEndOfStreamProfile(const karabo::xms::InputChannel::Pointer& input);

        std::vector<unsigned long long> m_transferTimes;
    };
} // namespace karabo

#endif /* PIPE_RECEIVER_DEVICE_HH */
