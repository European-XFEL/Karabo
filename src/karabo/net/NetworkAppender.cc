/* 
 * File:   NetworkAppender.cc
 * Author: irinak
 * 
 * Created on September 21, 2011, 4:39 PM
 */

#include <iostream>

#include <krb_log4cpp/LoggingEvent.hh>
#include <karabo/util/Hash.hh>

#include "NetworkAppender.hh"

using namespace krb_log4cpp;
using namespace karabo::util;

namespace karabo {
    namespace net {


        NetworkAppender::NetworkAppender(const std::string& name, const karabo::net::BrokerChannel::Pointer& channel) :
        LayoutAppender(name), m_channel(channel), m_ok(true)
        {
            // Time format should match "yyyy-MM-dd hh:mm:ss" as GUI expects in
            // logwidget.py:
            m_timeLayout.setConversionPattern("%d{%F %H:%M:%S}");
            m_priorityLayout.setConversionPattern("%p"); // DEBUG, INFO, WARN or ERROR
            m_categoryLayout.setConversionPattern("%c"); // deviceId
            m_messageLayout.setConversionPattern("%m");  // message text

            // Start thread
            m_thread = boost::thread(boost::bind(&karabo::net::NetworkAppender::checkLogCache, this));
        }


        NetworkAppender::~NetworkAppender() {
            close();

            // Stop checkLogCache and join thread
            m_ok = false;
            m_thread.join();
        }


        void NetworkAppender::close() {
        }


        void NetworkAppender::_append(const LoggingEvent& event) {
            boost::mutex::scoped_lock lock(m_mutex);
            // Add message as Hash to cache - since emplace_back is C++11,
            // we first push_back an empty one and fill it in-place instead
            // of creating one and push_back (i.e. copy) it...
            m_logCache.push_back(util::Hash());
            util::Hash& message = m_logCache.back();
            // The keys here are expected by the GUI in logwidget.py:
            message.set("timestamp", m_timeLayout.format(event));
            message.set("type", m_priorityLayout.format(event));
            message.set("category", m_categoryLayout.format(event));
            message.set("message", m_messageLayout.format(event));
        }


        void NetworkAppender::checkLogCache() {
            try {
                while (m_ok) {
                    writeNow();
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                }

            } catch (const karabo::util::Exception& e) {
                // Would be better to use the macro to the message into the
                // log file - would need KARABO_CLASSINFO in .hh ?
                // GF FIXME
                // KARABO_LOG_FRAMEWORK_ERROR << "Writing failed: " << e;
                std::cerr << "NetworkAppender: Writing failed: " << e << std::endl;
            }
        }


        void NetworkAppender::writeNow() {

            boost::mutex::scoped_lock lock(m_mutex);
            if (m_logCache.empty()) return;

            util::Hash header("target", "log");
            util::Hash data("messages", m_logCache);
            m_channel->write(header, data);
            m_logCache.clear();
        }


        bool NetworkAppender::reopen() {
            return true;
        }
    }
}

