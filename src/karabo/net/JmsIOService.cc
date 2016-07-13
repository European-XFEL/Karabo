/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 24, 2011, 10:37 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "JmsIOService.hh"
#include "JmsChannel.hh"

namespace karabo {
    namespace net {

        //KARABO_REGISTER_FACTORY_CC(AbstractIOService, JmsIOService)

        //int JmsIOService::m_threadCount = 0;


        void JmsIOService::run() {
            m_status = RUNNING;
            while (activateRegisteredTextMessageHandlers() || activateRegisteredBinaryMessageHandlers() || activateRegisteredWaitHandlers()) {
                m_threadGroup.join_all();
            }
            m_status = IDLE;
        }


        void JmsIOService::work() {
            m_status = WORKING;
            activateRegisteredTextMessageHandlers();
            activateRegisteredBinaryMessageHandlers();
            activateRegisteredWaitHandlers();
            while (m_status != STOPPED) {
                boost::this_thread::sleep(boost::posix_time::seconds(2));
            }
        }


        bool JmsIOService::activateRegisteredTextMessageHandlers() {
            boost::mutex::scoped_lock lock(m_mutex);
            if (m_textMessageChannels.empty()) return false;
            for (size_t i = 0; i < m_textMessageChannels.size(); ++i) {
                m_threadGroup.create_thread(boost::bind(&karabo::net::JmsChannel::listenForTextMessages, m_textMessageChannels[i]));
            }
            m_textMessageChannels.clear();
            return true;
        }


        bool JmsIOService::activateRegisteredBinaryMessageHandlers() {
            boost::mutex::scoped_lock lock(m_mutex);
            if (m_binaryMessageChannels.empty()) return false;
            for (size_t i = 0; i < m_binaryMessageChannels.size(); ++i) {
                m_threadGroup.create_thread(boost::bind(&karabo::net::JmsChannel::listenForBinaryMessages, m_binaryMessageChannels[i]));
            }
            m_binaryMessageChannels.clear();
            return true;
        }


        bool JmsIOService::activateRegisteredWaitHandlers() {
            boost::mutex::scoped_lock lock(m_mutex);
            if (m_waitHandlers.empty()) return false;
            for (size_t i = 0; i < m_waitHandlers.size(); ++i) {
                m_threadGroup.create_thread(boost::bind(&karabo::net::JmsChannel::deadlineTimer, m_waitHandlers[i].get<0>(), m_waitHandlers[i].get<1>(), m_waitHandlers[i].get<2>()));
            }
            m_waitHandlers.clear();
            return true;
        }


        void JmsIOService::stop() {
            m_status = STOPPED;
            m_threadGroup.join_all();
            m_status = IDLE;
        }


        bool JmsIOService::isStopped() {
            return m_status == STOPPED;
        }


        bool JmsIOService::isRunning() {
            return m_status == RUNNING;
        }


        bool JmsIOService::isWorking() {
            return m_status == WORKING;
        }


        void JmsIOService::registerTextMessageChannel(JmsChannel* channel) {
            //std::cout << "Registering thread No.: " << m_threadCount++ << std::endl;
            if (m_status == IDLE || m_status == STOPPED || m_status == RUNNING) {
                m_textMessageChannels.push_back(channel);
            } else if (m_status == WORKING) {
                m_threadGroup.create_thread(boost::bind(&karabo::net::JmsChannel::listenForTextMessages, channel));
            }
        }


        void JmsIOService::registerBinaryMessageChannel(JmsChannel* channel) {
            //std::cout << "Registering thread No.: " << m_threadCount++ << std::endl;
            if (m_status == IDLE || m_status == STOPPED || m_status == RUNNING) {
                m_binaryMessageChannels.push_back(channel);
            } else if (m_status == WORKING) {
                m_threadGroup.create_thread(boost::bind(&karabo::net::JmsChannel::listenForBinaryMessages, channel));
            }
        }


        void JmsIOService::registerWaitChannel(JmsChannel* channel, const WaitHandler& handler, int milliseconds) {
            //std::cout << "Registering thread No.: " << m_threadCount++ << std::endl;
            if (m_status == IDLE || m_status == STOPPED || m_status == RUNNING) {
                m_waitHandlers.push_back(boost::tuple<JmsChannel*, WaitHandler, int>(channel, handler, milliseconds));
            } else if (m_status == WORKING) {
                m_threadGroup.create_thread(boost::bind(&karabo::net::JmsChannel::deadlineTimer, channel, handler, milliseconds));
            }
        }
    }
}
