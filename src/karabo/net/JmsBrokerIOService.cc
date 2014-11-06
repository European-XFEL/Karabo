/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 24, 2011, 10:37 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "JmsBrokerIOService.hh"
#include "JmsBrokerChannel.hh"

namespace karabo {
    namespace net {


        KARABO_REGISTER_IN_FACTORY(AbstractIOService, JmsBrokerIOService)

        //int JmsBrokerIOService::m_threadCount = 0;

        void JmsBrokerIOService::run() {
            m_status = RUNNING;
            while (activateRegisteredMessageReceivers() || activateRegisteredWaitHandlers()) {
                m_threadGroup.join_all(); // Whilst this blocks, new message handlers can be registered. If no one was registered the while will return.
            }
            m_status = IDLE;
        }


        void JmsBrokerIOService::work() {
            m_status = WORKING;
            activateRegisteredMessageReceivers();
            activateRegisteredWaitHandlers();
            while (m_status != STOPPED) {
                boost::this_thread::sleep(boost::posix_time::seconds(2));
            }
            m_threadGroup.join_all();
            m_status = IDLE;
        }


        bool JmsBrokerIOService::activateRegisteredMessageReceivers() {
            boost::mutex::scoped_lock lock(m_mutex);
            if (m_messageReceivers.empty()) return false;
            for (size_t i = 0; i < m_messageReceivers.size(); ++i) {
                m_threadGroup.create_thread(m_messageReceivers[i]);
            }
            m_messageReceivers.clear();
            return true;
        }


        bool JmsBrokerIOService::activateRegisteredWaitHandlers() {
            boost::mutex::scoped_lock lock(m_mutex);
            if (m_waitHandlers.empty()) return false;
            for (size_t i = 0; i < m_waitHandlers.size(); ++i) {
                m_threadGroup.create_thread(boost::bind(&karabo::net::JmsBrokerChannel::deadlineTimer, m_waitHandlers[i].get<0>(), m_waitHandlers[i].get<1>(), m_waitHandlers[i].get<2>(), m_waitHandlers[i].get<3>()));
            }
            m_waitHandlers.clear();
            return true;
        }


        void JmsBrokerIOService::stop() {
            m_status = STOPPED;
        }


        bool JmsBrokerIOService::isStopped() {
            return m_status == STOPPED;
        }


        bool JmsBrokerIOService::isRunning() {
            return m_status == RUNNING;
        }


        bool JmsBrokerIOService::isWorking() {
            return m_status == WORKING;
        }


        void JmsBrokerIOService::registerMessageReceiver(const boost::function<void ()>& function) {
            //std::cout << "Registering thread No.: " << m_threadCount++ << std::endl;
            if (m_status == IDLE || m_status == STOPPED || m_status == RUNNING) {
                m_messageReceivers.push_back(function);
            } else if (m_status == WORKING) {
                m_threadGroup.create_thread(function);
            }
        }


        void JmsBrokerIOService::registerWaitChannel(JmsBrokerChannel* channel, const BrokerChannel::WaitHandler& handler, int milliseconds, const std::string& id) {
            //std::cout << "Registering thread No.: " << m_threadCount++ << std::endl;
            if (m_status == IDLE || m_status == STOPPED || m_status == RUNNING) {
                m_waitHandlers.push_back(boost::tuple<JmsBrokerChannel*, BrokerChannel::WaitHandler, int, std::string>(channel, handler, milliseconds, id));
            } else if (m_status == WORKING) {
                m_threadGroup.create_thread(boost::bind(&karabo::net::JmsBrokerChannel::deadlineTimer, channel, handler, milliseconds, id));
            }
        }
    }
}
