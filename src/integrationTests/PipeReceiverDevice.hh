/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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

    class PipeReceiverDevice : public karabo::core::Device<> {
       public:
        KARABO_CLASSINFO(PipeReceiverDevice, "PipeReceiverDevice", "2.0")

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        PipeReceiverDevice(const karabo::util::Hash& config);
        virtual ~PipeReceiverDevice() {}

       private:
        void initialization();

        void onInput(const karabo::xms::InputChannel::Pointer& input);

        void onData(const karabo::util::Hash& data, const xms::InputChannel::MetaData& metaData);
        void onInputProfile(const xms::InputChannel::Pointer& input);
        void reset();

        void onEndOfStream(const karabo::xms::InputChannel::Pointer& input);
        void onEndOfStreamProfile(const karabo::xms::InputChannel::Pointer& input);

        std::vector<unsigned long long> m_transferTimes;
    };
} // namespace karabo

#endif /* PIPE_RECEIVER_DEVICE_HH */
