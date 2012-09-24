/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 24, 2011, 10:37 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_NET_JMSIOSERVICE_HH
#define	EXFEL_NET_JMSIOSERVICE_HH

#include "AbstractIOService.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    namespace net {

        // Forward
        class Channel;
        class JmsChannel;

        /**
         * The JmsIOService class.
         */
        class JmsIOService : public AbstractIOService {
        public:

            typedef boost::shared_ptr<Channel> ChannelPointer;
            typedef boost::function<void (ChannelPointer) > WaitHandler;

            EXFEL_CLASSINFO(JmsIOService, "Jms", "1.0")

            JmsIOService() : m_status(IDLE) {
            }

            virtual ~JmsIOService() {
            }

            void run();

            void work();

            void stop();

            bool isStopped();

            bool isRunning();

            bool isWorking();

            void registerTextMessageChannel(JmsChannel* channel);

            void registerBinaryMessageChannel(JmsChannel* channel);

            void registerWaitChannel(JmsChannel* channel, const WaitHandler& handler, int milliseconds);

        private: // members

            enum IOServiceStatus {
                IDLE,
                STOPPED,
                RUNNING,
                WORKING
            };

            IOServiceStatus m_status;
            boost::thread_group m_threadGroup;
            std::vector<JmsChannel*> m_textMessageChannels;
            std::vector<JmsChannel*> m_binaryMessageChannels;
            std::vector<boost::tuple<JmsChannel*, WaitHandler, int> > m_waitHandlers;
            boost::mutex m_mutex;
            
            //static int m_threadCount;

        private: // functions

            bool activateRegisteredTextMessageHandlers();

            bool activateRegisteredBinaryMessageHandlers();
            
            bool activateRegisteredWaitHandlers();

        };

    } // namespace net
} // namespace exfel

#endif 

