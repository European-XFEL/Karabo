/*
 * File:   SnmpIOService.cc
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 12:21 PM
 */

#include "SnmpIOService.hh"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

using namespace std;
using namespace exfel::util;

namespace exfel {
    namespace net {

        EXFEL_REGISTER_FACTORY_CC(AbstractIOService, SnmpIOService)

        void SnmpIOService::run() {


            while (m_expectedReplies) {
                int maxfd = 0;
                int block = 1;
                fd_set readfds;
                struct timeval timeout;

                FD_ZERO(&readfds);
                ::snmp_select_info(&maxfd, &readfds, &timeout, &block);
                int nfds = ::select(maxfd, &readfds, NULL, NULL, block ? NULL : &timeout);
                if (nfds > 0) { // one ore more file descriptors 'ready'
                    ::snmp_read(&readfds);
                } else if (nfds == 0) { // no file descriptors 'ready'
                    ::snmp_timeout();
                } else { // exception in 'select'
                    throw IO_EXCEPTION(string("Native 'select' failed: ") + ::strerror(errno));
                }
            }
        }

        void SnmpIOService::work() {
            increaseReplyCount();
            run();
        }

        void SnmpIOService::stop() {
            decreaseReplyCount();
        }

        void SnmpIOService::increaseReplyCount() {
            m_expectedReplies++;
        }

        void SnmpIOService::decreaseReplyCount() {
            if (m_expectedReplies > 0) {
                m_expectedReplies--;
            }
        }
    }
}

