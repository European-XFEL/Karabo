/* 
 * File:   PipeReceiverDevice.hh
 * Author: flucke
 *
 * Created on October 14, 2016, 2:30 PM
 */

#ifndef PIPERECEIVERDEVICE_HH
#define	PIPERECEIVERDEVICE_HH

#include "karabo/core/Device.hh"

namespace karabo {

    namespace util {
        class Schema;
        class Hash;
    }
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
        virtual ~PipeReceiverDevice();

    private:
        void initialization();

        void onInput(karabo::xms::InputChannel& input);

        void onData(const karabo::util::Hash& data);

        void onEndOfStream(karabo::xms::InputChannel& input);
    };
}
#endif	/* PIPERECEIVERDEVICE_HH */

