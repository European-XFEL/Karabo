/*
 * $Id: AlarmTester.hh 7651 2016-06-16 11:46:40Z haufs $
 *
 * Author: <steffen.hauf@xfel.eu>
 *
 * Created on June, 2016, 03:03 PM
 *
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

#ifndef KARABO_TCPADAPTERDEVICE_HH
#define KARABO_TCPADAPTERDEVICE_HH

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread.hpp>
#include <chrono>
#include <future>
#include <thread>

#include "karabo/data/types/Exception.hh"
#include "karabo/karabo.hpp"
#include "karabo/net/TcpChannel.hh"


/**
 * The main Karabo namespace
 */
namespace karabo {

    class TcpAdapter : public std::enable_shared_from_this<TcpAdapter> {
       public:
        typedef std::shared_ptr<boost::lockfree::spsc_queue<karabo::data::Hash> > QueuePtr;

        /**
         * Constructor for a TcpAdapter, takes a config Hash as parameter
         * @param config: should contain "port" to connect to (unsigned long long)
         * and optionally "debug" (bool).
         */
        TcpAdapter(const karabo::data::Hash& config);

        virtual ~TcpAdapter();

        /**
         * Return a list of all messages of a given type received by this TcpAdapter
         * @param type: type of message
         * @return a vector of Hashes containing the messages.
         */
        std::vector<karabo::data::Hash> getAllMessages(const std::string& type);

        /**
         * Clear list of all messages of a given type received by this TcpAdapter
         * @param type: type of message - if empty (default!), clear all types
         */
        void clearAllMessages(const std::string& type = "");

        /**
         * Get the next nMessages messages of a given type, that this TcpAdapter receives as a queue
         * @param type: type of messages
         * @param nMessages: number of messages to wait for
         * @param triggeringFunction: function to call before waiting on messages. Signature is void(), can be a lambda,
         * the default is an empty function
         * @param timeout: timeout (in ms) for waiting for messages. Set to 0 for infinite timeout, defaults to 10000
         * @return
         */
        template <typename F>
        [[nodiscard]]
        QueuePtr getNextMessages(
              const std::string& type, size_t nMessages, F&& triggeringFunction = [] {}, size_t timeout = 10000) {
            {
                std::unique_lock lock(m_queueAccessMutex);
                m_nextMessageQueues[type] = std::shared_ptr<boost::lockfree::spsc_queue<karabo::data::Hash> >(
                      new boost::lockfree::spsc_queue<karabo::data::Hash>(nMessages));
            }

            // call the function which triggers the expected messages
            triggeringFunction();

            std::shared_lock lock(m_queueAccessMutex);
            const size_t waitTime = 100; // ms
            const size_t maxLoops = std::ceil(timeout / waitTime);
            size_t i = 0;
            do {
                if (i == maxLoops) {
                    const std::string msg("Waiting on " + karabo::data::toString(nMessages) + " messages of type '" +
                                          type + "' timed out!");
                    throw KARABO_TIMEOUT_EXCEPTION(msg);
                }
                i++;
                std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
                if (m_debug) {
                    std::clog << "Have " << m_nextMessageQueues[type]->read_available() << " of " << nMessages
                              << " in queue for '" << type << "'!" << std::endl;
                }
            } while (m_nextMessageQueues[type]->read_available() < nMessages);


            return m_nextMessageQueues[type];
        };


        /**
         * Check if connection was sucessfully established
         * @return
         */
        bool connected() const;

        /**
         * Send a message to the server
         * @param message: a hash containing the message
         * @param block: if true, block until the onWriteComplete handler has been called
         */
        void sendMessage(const karabo::data::Hash& message, bool block = true);

        /**
         * Disconnect adapter
         */
        void disconnect();

        /**
         * Merge the given argument (default: empty) to the default login message, send it and wait for reply
         */
        void login(const karabo::data::Hash& extraLoginData = karabo::data::Hash());

        /**
         * Waits for a callback to return true when executed on messages of a specifc type.
         * NOTE: the ``callback`` function will be called under a mutex protection.
         * If the function attempts locking the mutex, this will result in a deadlock!
         *
         * @param type: the type of the incoming messages
         * @param callback: the function to be executed on the messages of type `type`
         * @param timeoutInMs: timeout in ms.
         */
        std::future_status waitFor(const std::string& type,
                                   const std::function<bool(const karabo::data::Hash&)>& callback,
                                   unsigned int timeoutInMs);

       private:
        void onConnect(const karabo::net::ErrorCode& ec, int timeout, int repetition,
                       const karabo::net::Channel::Pointer& channel);
        void waitHandler(const karabo::net::ErrorCode& ec, int timeout, int repetition);
        void onRead(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel, karabo::data::Hash& info);
        void onError(const karabo::net::ErrorCode& errorCode, karabo::net::Channel::Pointer channel);
        void onWriteComplete(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, size_t id);


       private:
        karabo::net::Connection::Pointer m_dataConnection;
        std::map<std::string, std::vector<karabo::data::Hash> > m_messages;
        std::map<std::string, bool> m_messageConditions;
        mutable std::shared_mutex m_messageAccessMutex;
        mutable std::shared_mutex m_queueAccessMutex;
        mutable std::map<std::string, std::shared_ptr<boost::lockfree::spsc_queue<karabo::data::Hash> > >
              m_nextMessageQueues;
        boost::asio::steady_timer m_deadline;
        bool m_debug;
        size_t m_MessageId;
        mutable std::mutex m_writeConditionMutex;
        std::condition_variable m_writeCondition;
        std::atomic_size_t m_writeWaitForId;
        karabo::net::TcpChannel::Pointer m_channel;
        static const karabo::data::Hash k_defaultLoginData;
        mutable std::shared_mutex m_callbackMutex;
        std::function<void(const karabo::data::Hash&)> m_callback;
    };
} // namespace karabo

#endif
