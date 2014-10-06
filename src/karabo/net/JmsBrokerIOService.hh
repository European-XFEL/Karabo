/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 24, 2011, 10:37 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_NET_JMSBROKERIOSERVICE_HH
#define	KARABO_NET_JMSBROKERIOSERVICE_HH

#include "AbstractIOService.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace net {

        // Forward
        class BrokerChannel;
        class JmsBrokerChannel;

        /**
         * The JmsBrokerIOService class.
         */
        class JmsBrokerIOService : public AbstractIOService {
        public:

            typedef boost::shared_ptr<BrokerChannel> BrokerChannelPointer;
            typedef boost::function<void (BrokerChannelPointer) > WaitHandler;

            KARABO_CLASSINFO(JmsBrokerIOService, "Jms", "1.0")

            JmsBrokerIOService() : m_status(IDLE) {
            }

            virtual ~JmsBrokerIOService() {
            }

            void run();

            void work();

            void stop();

            bool isStopped();

            bool isRunning();

            bool isWorking();
            
            void registerMessageReceiver(const boost::function<void ()>& function);

            void registerTextMessageChannel(JmsBrokerChannel* channel);

            void registerBinaryMessageChannel(JmsBrokerChannel* channel);

            void registerWaitChannel(JmsBrokerChannel* channel, const WaitHandler& handler, int milliseconds);

        private: // members

            enum IOServiceStatus {
                IDLE,
                STOPPED,
                RUNNING,
                WORKING
            };

            IOServiceStatus m_status;
            boost::thread_group m_threadGroup;
            std::vector<JmsBrokerChannel*> m_textMessageChannels;
            std::vector<JmsBrokerChannel*> m_binaryMessageChannels;
            std::vector<boost::function<void ()> > m_messageReceivers;
            std::vector<boost::tuple<JmsBrokerChannel*, WaitHandler, int> > m_waitHandlers;
            boost::mutex m_mutex;
            
        private: // functions

            bool activateRegisteredMessageReceivers();                      
            
            bool activateRegisteredWaitHandlers();

        };
    } 
}

#endif 

