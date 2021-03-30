/* 
 * File:   NetworkAppender.cc
 * Author: irinak
 * 
 * Created on September 21, 2011, 4:39 PM
 */

#include <boost/algorithm/string.hpp>
#include "NetworkAppender.hh"
#include "Logger.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/LeafElement.hh"
#include "karabo/util/ChoiceElement.hh"
#include "karabo/util/NodeElement.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/EventLoop.hh"
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

            CHOICE_ELEMENT(s).key("connection")
                    .displayedName("Connection")
                    .appendNodesOfConfigurationBase<karabo::net::Broker>()
                    .assignmentOptional().noDefaultValue()
                    .expertAccess()
                    .commit();

            UINT32_ELEMENT(s).key("interval")
                    .displayedName("Interval")
                    .description("Time between subsequent sending of messages")
                    .assignmentOptional().defaultValue(1000u)
                    .unit(Unit::SECOND)
                    .metricPrefix(MetricPrefix::MILLI)
                    .commit();

            UINT32_ELEMENT(s).key("maxNumMessages")
                    .displayedName("Max. Num. Messages")
                    .description("Maximum number of messages - if mor per interval arrive, they are dropped")
                    .assignmentOptional().defaultValue(1000u)
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
            LayoutAppender(config.get<std::string>("name")),
            m_producer(),
            m_interval(config.get<unsigned int>("interval")),
            m_maxMessages(config.get<unsigned int>("maxNumMessages")),
            m_timer(EventLoop::getIOService()) {

            // If we created the connection ourselves we are still disconnected
            m_producer = Configurator<Broker>::createChoice("connection", config);
            if (!m_producer->isConnected()) m_producer->connect();

            // Time format should be ISO8601, for real, not the
            // log4cpp crippled version (missing T)
            m_timeLayout.setConversionPattern("%d{%Y-%m-%dT%H:%M:%S.%l}");
            m_priorityLayout.setConversionPattern("%p"); // DEBUG, INFO, WARN or ERROR
            m_categoryLayout.setConversionPattern("%c"); // deviceId
            m_messageLayout.setConversionPattern("%m"); // message text

            // Define a filter for this appender that filter out all messages coming from "framework" flavor of logging
            // IMPORTANT NOTE: Define NetworkFilter object as bare pointer because
            // Log4cpp will take ownership and deal with memory management (proper delete)
            this->setFilter(new NetworkFilter());

            startLogWriting();
        }


        Log4CppNetApp::~Log4CppNetApp() {
            // Take care that m_timer.cancel() really cancels something:
            // If not, our handler (checkLogCache) is already posted to the event loop and will access data members
            // and bind a bare this pointer again - all this must not happen after this destructor is finished.
            while (m_timer.cancel() == 0) {
                boost::this_thread::sleep(boost::posix_time::milliseconds(1));
            }
            close();
        }


        void Log4CppNetApp::close() {
            m_producer.reset();
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


        void Log4CppNetApp::startLogWriting() {
            m_timer.expires_from_now(boost::posix_time::milliseconds(m_interval));
            // We cannot use bind_weak here: Even if Log4CppNetApp inherits from enable_shared_from_this and
            // NetworkAppender keeps a shared_ptr to this Log4CppNetApp, this does not help: NetworkAppender does not
            // live long enough to be responsible for the Log4CppNetApp it creates.
            // As way out the destructor of Log4CppNetApp takes care that there is no problem using boost::bind with
            // a bare 'this' pointer.
            m_timer.async_wait(boost::bind(&Log4CppNetApp::checkLogCache, this, boost::asio::placeholders::error));
        }


        void Log4CppNetApp::checkLogCache(const boost::system::error_code& e) {

            if (e) {
                // cancelled
                return;
            }

            try {
                writeNow();
            } catch (const karabo::util::Exception& e) {
                boost::mutex::scoped_lock lock(m_mutex);
                KARABO_LOG_FRAMEWORK_ERROR << "Writing failed for "
                        << m_logCache.size() << " message(s): " << e;
                // Clean up, i.e. do not try to send again since messages
                // should anyway be in server log:
                m_logCache.clear();
            }

            // Fire again:
            startLogWriting();
        }


        void Log4CppNetApp::writeNow() {

            boost::mutex::scoped_lock lock(m_mutex);
            if (m_logCache.empty()) return;

            if (m_logCache.size() > m_maxMessages) {
                KARABO_LOG_FRAMEWORK_ERROR << "Send only maximum of " << m_maxMessages
                        << " messages instead of the " << m_logCache.size() << " accumulated ones.";
                m_logCache.resize(m_maxMessages);
            }
            auto header = Hash::Pointer(new Hash("target", "log"));
            auto body = Hash::Pointer(new Hash("messages", m_logCache));
            m_producer->write(m_producer->getDomain(), header, body, 0, 0);
            m_logCache.clear();
        }


        bool Log4CppNetApp::reopen() {
            return true;
        }


        NetworkFilter::NetworkFilter() : krb_log4cpp::Filter() {
        }


        NetworkFilter::~NetworkFilter() {
        }


        krb_log4cpp::Filter::Decision NetworkFilter::_decide(const krb_log4cpp::LoggingEvent& event) {
            // Deny all logging events coming from categories registered in FRAMEWORK category name set
            if (Logger::isInCategoryNameSet(event.categoryName)) return krb_log4cpp::Filter::DENY;
            return krb_log4cpp::Filter::NEUTRAL;
        }
    }
}

