/*
 * File:   CentralLogging.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on November 9, 2015, 12:23 PM
 */

#ifndef KARABO_CORE_CENTRALLOGGING_HH
#define	KARABO_CORE_CENTRALLOGGING_HH

#include "karabo/core/Device.hh"
#include "karabo/util/Hash.hh"
#include "karabo/net/JmsConsumer.hh"
/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace devices {

        class CentralLogging : public karabo::core::Device<> {

        public:

            KARABO_CLASSINFO(CentralLogging, "CentralLogging", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            CentralLogging(const karabo::util::Hash& input);

            virtual ~CentralLogging();

        private:

            void initialize();

            int determineLastIndex();

            int incrementLastIndex();

            void flushHandler(const boost::system::error_code& ec);

            void logHandler(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& data);

            /**
             * This device may not be locked
             * @return false
             */
            bool allowLock() const {
                return false;
            }

        private:

            unsigned int m_lastIndex;
            std::fstream m_logstream;
            boost::mutex m_streamMutex;
            karabo::net::JmsConsumer::Pointer m_loggerConsumer;
            boost::asio::deadline_timer m_timer;
        };
    }
}


#endif	/* KARABO_CORE_CENTRALLOGGING_HH */

