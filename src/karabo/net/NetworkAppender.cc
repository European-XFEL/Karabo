/* 
 * File:   NetworkAppender.cc
 * Author: irinak
 * 
 * Created on September 21, 2011, 4:39 PM
 */

#include <krb_log4cpp/LoggingEvent.hh>
#include <karabo/util/Hash.hh>
#include "NetworkAppender.hh"

using namespace krb_log4cpp;
using namespace karabo::util;

namespace karabo {
    namespace net {


        NetworkAppender::NetworkAppender(const std::string& name, const karabo::net::BrokerChannel::Pointer& channel) :
        LayoutAppender(name), m_channel(channel), m_ok(true) {

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
            m_logCache += _getLayout().format(event) + "#";
        }


        void NetworkAppender::checkLogCache() {
            try {
                while (m_ok) {
                    writeNow();
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                }

            } catch (const karabo::util::Exception& e) {
                std::cout << e;
            }
        }


        void NetworkAppender::writeNow() {

            boost::mutex::scoped_lock lock(m_mutex);
            if (m_logCache.empty()) return;

            Hash header("target", "log");
            m_channel->write(header, m_logCache);
            m_logCache.clear();
        }


        bool NetworkAppender::reopen() {
            return true;
        }
    }
}

