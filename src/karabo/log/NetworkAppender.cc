/* 
 * File:   NetworkAppender.cc
 * Author: irinak
 * 
 * Created on September 21, 2011, 4:39 PM
 */

#include "NetworkAppender.hh"
#include "Logger.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/LeafElement.hh"
#include "karabo/util/NodeElement.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/net/JmsConnection.hh"
#include <krb_log4cpp/LoggingEvent.hh>
#include <iostream>

using namespace krb_log4cpp;
using namespace karabo::util;
using namespace karabo::net;

namespace karabo {
    namespace log {

        KARABO_REGISTER_FOR_CONFIGURATION(NetworkAppender)

        void NetworkAppender::expectedParameters(karabo::util::Schema& s) {

            STRING_ELEMENT(s).key("name")
                    .displayedName("Name")
                    .description("Name of the appender")
                    .assignmentOptional().defaultValue("network")
                    .commit();

            NODE_ELEMENT(s).key("connection")
                    .appendParametersOf<JmsConnection>()
                    .commit();

            STRING_ELEMENT(s).key("topic")
                    .displayedName("Topic")
                    .description("The topic on which the log messages should be published")
                    .assignmentOptional().defaultValue("karabo_log")
                    .commit();
        }


        NetworkAppender::NetworkAppender(const karabo::util::Hash& config) {
            // This raw pointer will be handed to a log4cpp category object (functionality of Logger class)
            // Log4cpp will take ownership and deal with memory management (proper delete)
            m_appender = new Log4CppNetApp(config);
        }


        krb_log4cpp::Appender* NetworkAppender::getAppender() {
            return m_appender;
        }


        Log4CppNetApp::Log4CppNetApp(const karabo::util::Hash& config) :
            LayoutAppender(config.get<string>("name")),
            m_connection(Configurator<JmsConnection>::createNode("connection", config)),
            m_topic(config.get<string>("topic")),
            m_ok(true) {

            // If we created the connection ourselves we are still disconnected
            if (!m_connection->isConnected()) m_connection->connect();
            m_producer = m_connection->createProducer();

            // Time format should be ISO8601, for real, not the
            // log4cpp crippled version (missing T)
            m_timeLayout.setConversionPattern("%d{%Y-%m-%dT%H:%M:%S.%l}");
            m_priorityLayout.setConversionPattern("%p"); // DEBUG, INFO, WARN or ERROR
            m_categoryLayout.setConversionPattern("%c"); // deviceId
            m_messageLayout.setConversionPattern("%m"); // message text

            // Start thread
            m_thread = boost::thread(boost::bind(&karabo::log::Log4CppNetApp::checkLogCache, this));
        }


        Log4CppNetApp::~Log4CppNetApp() {
            close();

            // Stop checkLogCache and join thread
            m_ok = false;
            m_thread.join();
        }


        void Log4CppNetApp::close() {
        }


        void Log4CppNetApp::_append(const LoggingEvent& event) {
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


        void Log4CppNetApp::checkLogCache() {
            while (m_ok) {
                try {
                    writeNow();
                } catch (const karabo::util::Exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Writing failed for "
                            << m_logCache.size() << " message(s): " << e;
                    // Clean up, i.e. do not try to send again since messages
                    // should anyway be in server log:
                    boost::mutex::scoped_lock lock(m_mutex);
                    m_logCache.clear();
                }
                boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            }
        }


        void Log4CppNetApp::writeNow() {

            boost::mutex::scoped_lock lock(m_mutex);
            if (m_logCache.empty()) return;

            auto header = Hash::Pointer(new Hash("target", "log"));
            auto body = Hash::Pointer(new Hash("messages", m_logCache));
            m_producer->write(m_topic, header, body, 0);
            m_logCache.clear();
        }


        bool Log4CppNetApp::reopen() {
            return true;
        }
    }
}

