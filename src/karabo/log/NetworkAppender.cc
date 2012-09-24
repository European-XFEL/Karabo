/* 
 * File:   NetworkAppender.cc
 * Author: irinak
 * 
 * Created on September 21, 2011, 4:39 PM
 */

#include <log4cpp/LoggingEvent.hh>
#include <karabo/util/Hash.hh>
#include "NetworkAppender.hh"

using namespace log4cpp;
using namespace exfel::util;

namespace exfel {
    namespace log {

        NetworkAppender::NetworkAppender(const std::string& name, const exfel::net::BrokerChannel::Pointer& channel) :
        LayoutAppender(name), m_channel(channel), m_ok(true) {
            
            // Start thread
            m_thread = boost::thread(boost::bind(&exfel::log::NetworkAppender::checkLogCache, this));
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
//            Hash header;
//            header.set("categoryName", event.categoryName );
//            header.set("priority", Priority::getPriorityName(event.priority) );
//            header.set("target", "log");
//            m_channel->write(_getLayout().format(event), header);
            //std::cout << "####" << _getLayout().format(event);
            boost::mutex::scoped_lock lock(m_mutex);
            m_logCache += _getLayout().format(event) + "#";
        }
        
        void NetworkAppender::checkLogCache() {
            try {
                while (m_ok) {
                    writeNow();
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                }
                
            } catch(const exfel::util::Exception& e) {
                std::cout << e;
            }
        }
        
        void NetworkAppender::writeNow() {
            
            boost::mutex::scoped_lock lock(m_mutex);  
            if (m_logCache.empty()) return;
            
            Hash header("target", "log");    
            m_channel->write(m_logCache, header);
            m_logCache.clear();
        }

        bool NetworkAppender::reopen() {
            return true;
        }

    }
}

