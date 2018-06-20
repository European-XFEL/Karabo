/*
 * $Id: AlarmTester.hh 7651 2016-06-16 11:46:40Z haufs $
 *
 * Author: <steffen.hauf@xfel.eu>
 *
 * Created on June, 2016, 03:03 PM
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_TCPADAPTERDEVICE_HH
#define KARABO_TCPADAPTERDEVICE_HH

#include "karabo/karabo.hpp"
#include "karabo/net/TcpChannel.hh"
#include "karabo/util/Exception.hh"
#include <boost/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <future>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class TcpAdapter {
        
        

    public:
        
        typedef boost::shared_ptr<boost::lockfree::spsc_queue<karabo::util::Hash> > QueuePtr;

        /**
         * Constructor for a TcpAdapter, takes a config Hash as parameter
         * @param config: should contain "port" to connect to (unsigned long long)
         * and optionally "debug" (bool).
         */
        TcpAdapter(const karabo::util::Hash& config);
        
        virtual ~TcpAdapter();
        
        /**
         * Return a list of all messages of a given type received by this TcpAdapter
         * @param type: type of message
         * @return a vector of Hashes containing the messages.
         */
        std::vector<karabo::util::Hash> getAllMessages(const std::string& type);

        /**
         * Clear list of all messages of a given type received by this TcpAdapter
         * @param type: type of message - if empty (default!), clear all types
         */
        void clearAllMessages(const std::string& type = "");

        /**
         * Get the next nMessages messages of a given type, that this TcpAdapter receives as a queue
         * @param type: type of messages
         * @param nMessages: number of messages to wait for
         * @param triggeringFunction: function to call before waiting on messages. Signature is void(), can be a lambda, the default is an empty function
         * @param timeout: timeout (in ms) for waiting for messages. Set to 0 for infinite timeout, defaults to 5000
         * @return 
         */
        template<typename F>
        QueuePtr getNextMessages(const std::string& type, size_t nMessages, F&& triggeringFunction = []{}, size_t timeout=5000) {


            {
                boost::unique_lock<boost::shared_mutex> lock(m_queueAccessMutex);
                m_nextMessageQueues[type] = boost::shared_ptr<boost::lockfree::spsc_queue<karabo::util::Hash> >(new boost::lockfree::spsc_queue<karabo::util::Hash>(nMessages));
                
            }

            //call the function which triggers the expected messages
            triggeringFunction();
            
            boost::shared_lock<boost::shared_mutex> lock(m_queueAccessMutex);
            const size_t waitTime = 100; //ms
            const size_t maxLoops = std::ceil(timeout/waitTime);
            size_t i = 0;
            do {
                if (i == maxLoops) {
                    const std::string msg("Waiting on " + karabo::util::toString(nMessages)
                                          + " messages of type '" + type + "' timed out!");
                    throw KARABO_TIMEOUT_EXCEPTION(msg);
                }
                i++;
                boost::this_thread::sleep(boost::posix_time::milliseconds(waitTime));
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
        void sendMessage(const karabo::util::Hash & message, bool block = true);
        
        /**
         * Disconnect adapter
         */
        void disconnect();

    private:

        void onConnect(const karabo::net::ErrorCode& ec, int timeout, int repetition, const karabo::net::Channel::Pointer& channel);
        void waitHandler(const karabo::net::ErrorCode& ec, int timeout, int repetition);
        void onRead(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel, karabo::util::Hash& info);
        void onError(const karabo::net::ErrorCode& errorCode, karabo::net::Channel::Pointer channel);
        void onWriteComplete(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, size_t id);
        

    private:

        karabo::net::Connection::Pointer m_dataConnection;
        std::map<std::string, std::vector<karabo::util::Hash> >m_messages;
        std::map<std::string, bool> m_messageConditions;
        mutable boost::shared_mutex m_messageAccessMutex;
        mutable boost::shared_mutex m_queueAccessMutex;
        mutable std::map<std::string, boost::shared_ptr<boost::lockfree::spsc_queue<karabo::util::Hash> > >m_nextMessageQueues;
        boost::asio::deadline_timer m_deadline;
        bool m_debug;
        size_t m_MessageId;
        mutable boost::mutex m_writeConditionMutex;
        boost::condition_variable m_writeCondition;
        std::atomic_size_t m_writeWaitForId;
        karabo::net::TcpChannel::Pointer m_channel;

    };
}

#endif
