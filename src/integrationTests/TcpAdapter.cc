/*
 * $Id: AlarmTester.cc 7755 2016-06-24 14:10:56Z haufs $
 *
 * Author: <steffen.hauf@xfel.eu>
 * 
 * Created on June, 2016, 03:03 PM
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#include "TcpAdapter.hh"
#include <cassert>

using namespace std;
using namespace karabo::util;
using namespace karabo::core;
using namespace karabo::net;
using namespace karabo::io;
using namespace karabo::xms;

namespace karabo {


    const Hash TcpAdapter::k_defaultLoginData("type", "login", "username", "mrusp", "password", "12345", "version", karabo::util::Version::getKaraboVersion().getVersion());

    TcpAdapter::TcpAdapter(const karabo::util::Hash& config) : 
            m_deadline(karabo::net::EventLoop::getIOService()),
            m_MessageId(0) {
        
        Hash h;
        h.set("port", config.get<unsigned int>("port"));
        h.set("serializationType", "binary");
        m_dataConnection = Connection::create("Tcp", h);
        m_dataConnection->startAsync(boost::bind(&karabo::TcpAdapter::onConnect, this, _1, 500, 10, _2));
        m_debug = config.has("debug") ? config.get<bool>("debug") : false;
    }


    TcpAdapter::~TcpAdapter() {
        if (m_dataConnection) m_dataConnection->stop();
    }

    
    void TcpAdapter::onConnect(const karabo::net::ErrorCode& ec, int timeout, int repetition, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            onError(ec, channel);
            if (ec != boost::asio::error::eof && repetition >= 0) {

                m_deadline.expires_from_now(boost::posix_time::milliseconds(timeout));
                m_deadline.async_wait(boost::bind(&karabo::TcpAdapter::waitHandler, this, boost::asio::placeholders::error, timeout, repetition));
            }
            return;
        }
        m_channel = boost::dynamic_pointer_cast<TcpChannel>(channel);
        if (channel->isOpen()) channel->readAsyncHash(boost::bind(&karabo::TcpAdapter::onRead, this, _1, channel, _2));
    }

    void TcpAdapter::waitHandler(const karabo::net::ErrorCode& ec, int timeout, int repetition) {
        if (ec == boost::asio::error::operation_aborted) return;
        --repetition;
        timeout *= 2;
        
        if (repetition == 0) {
            if (m_debug) std::clog<<"Connecting failed. Timed out!"<<std::endl;

            return;
        };
            
        m_dataConnection->startAsync(boost::bind(&karabo::TcpAdapter::onConnect, this, _1, timeout, repetition, _2));
    }
    
    
    void TcpAdapter::onRead(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel, karabo::util::Hash& info) {
        if (e) {
            onError(e, channel);
            if (channel) channel->close();
            return;
        }

        try {
            // GUI communication scenarios
            if (m_debug) {
                std::clog <<"Received message: "<<info << std::endl;
            }

            {
                const std::string& type = info.has("type") ? info.get<std::string>("type") : "unspecified";
                {
                    boost::unique_lock<boost::shared_mutex> lock(m_messageAccessMutex);
                    std::vector<Hash>& messages = m_messages[type];
                    messages.push_back(info);
                }
                {
                    boost::shared_lock<boost::shared_mutex> lock(m_queueAccessMutex);
                    if(m_nextMessageQueues.find(type) != m_nextMessageQueues.end()) {
                        if(m_debug) std::clog << "Pushing to queue "<<type<<std::endl;
                        m_nextMessageQueues[type]->push(info);
                                              
                    }
                    
                }
  
                
            }
             
            if (channel->isOpen()) channel->readAsyncHash(boost::bind(&karabo::TcpAdapter::onRead, this, _1, channel, _2));
        } catch (const Exception& e) {
            std::cerr  << "Problem in onRead(): " << e.userFriendlyMsg() << std::endl;
            if (channel->isOpen()) channel->readAsyncHash(boost::bind(&karabo::TcpAdapter::onRead, this, _1, channel, _2));
           
        }
    }
    
    void TcpAdapter::onError(const karabo::net::ErrorCode& errorCode, karabo::net::Channel::Pointer channel) {
        try {

            if (m_debug) std::clog  << "onError : TCP socket got error : " << errorCode.value() << " -- \"" << errorCode.message() << "\",  Close connection to GuiServerDevice" <<std::endl;

        } catch (const Exception& e) {
            std::cerr << "Problem in onError(): " << e.userFriendlyMsg() << std::endl;
        } catch (const std::exception& se) {
            std::cerr << "Standard exception in onError(): " << se.what() << std::endl;
        }
    }
    

    void TcpAdapter::login() {
        QueuePtr messageQ = getNextMessages("systemTopology", 1, [&] {
            sendMessage(k_defaultLoginData);
        });
        // Clean received message from internal cache.
        Hash lastMessage;
        messageQ->pop(lastMessage);
    }
    
    
    std::vector<Hash> TcpAdapter::getAllMessages(const std::string& type) {
        boost::shared_lock<boost::shared_mutex> lock(m_messageAccessMutex);
        return m_messages[type];
    }


    void TcpAdapter::clearAllMessages(const std::string& type) {
        boost::shared_lock<boost::shared_mutex> lock(m_messageAccessMutex);
        if (type.empty()) {
            m_messages.clear();
        } else {
            m_messages[type].clear();
        }
    }


    bool TcpAdapter::connected() const {
        return m_channel && m_channel->isOpen();
    };
    
    void TcpAdapter::sendMessage(const karabo::util::Hash & message, bool block){
        if (!connected()) return;
        boost::unique_lock<boost::mutex> lock(m_writeConditionMutex);
        m_writeWaitForId = ++m_MessageId;
        m_channel->writeAsyncHash(message, boost::bind(&karabo::TcpAdapter::onWriteComplete, this, _1, m_channel, m_MessageId));
        if(block){
            m_writeCondition.wait(lock);
        }
    }

    void TcpAdapter::onWriteComplete(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, size_t id) {
        if (ec) {
            onError(ec, channel);
            if (channel) channel->close();
            boost::lock_guard<boost::mutex> lock(m_writeConditionMutex);
            m_writeCondition.notify_all();
            return;
        }
        
        if(m_writeWaitForId == id) {
            boost::lock_guard<boost::mutex> lock(m_writeConditionMutex);
            m_writeCondition.notify_all();
        }
    }
    
    void TcpAdapter::disconnect() {
        m_channel->close();
        m_dataConnection->stop();
    }
    
   
}
