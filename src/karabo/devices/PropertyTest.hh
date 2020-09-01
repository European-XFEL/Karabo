/* 
 * File:   PropertyTest.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on September 5, 2016, 11:08 AM
 */

#ifndef KARABO_DEVICES_PROPERTYTEST_HH
#define KARABO_DEVICES_PROPERTYTEST_HH

#include <boost/asio/deadline_timer.hpp>

#include "karabo/core/Device.hh"
#include "karabo/xms/InputChannel.hh"

namespace karabo {
    namespace util {
        class Schema;
        class Hash;
    }

    namespace devices {
        
        
        class NestedClass {
        public:
            
            KARABO_CLASSINFO(NestedClass, "NestedClass", "1.5")
            KARABO_CONFIGURATION_BASE_CLASS
                    
            static void expectedParameters(karabo::util::Schema& expected); 
                   
            NestedClass(const karabo::util::Hash& input);
            
            virtual ~NestedClass();
        };
        
        /**
         * @class PropertyTest
         * @brief The PropertyTest device includes all types Karabo knows about
         *        in it's expected parameter section. It is a test device to 
         *        assure changes to the framework do not result in broken types.
         */
        class PropertyTest : public karabo::core::Device<> {
        public:
            
            KARABO_CLASSINFO(PropertyTest, "PropertyTest", "1.5")
            
            static void expectedParameters(karabo::util::Schema& expected);
            
            PropertyTest(const karabo::util::Hash& config);
            
            ~PropertyTest();
            
        private:

            void initialize();

            void preReconfigure(karabo::util::Hash& incomingReconfiguration);

            void writeOutput();

            void writeOutputHandler(const boost::system::error_code& e);

            void startWritingOutput();

            void stopWritingOutput();

            void onData(const karabo::util::Hash& data, const karabo::xms::InputChannel::MetaData& meta);

            void resetChannelCounters();

            void eosOutput();

            void slotUpdateSchema();

            void node_increment();

            void node_reset();

            void replier(const karabo::xms::SignalSlotable::AsyncReply & areply);

            void slowSlot();

            bool m_writingOutput;
            boost::asio::deadline_timer m_writingOutputTimer;
        };
    } 
}

#endif	/* KARABO_DEVICES_PROPERTYTEST_HH */

