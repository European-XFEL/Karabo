/* 
 * File:   IOServiceWrap.hh
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 5, 2013, 11:16 AM
 */

#ifndef KARATHON_IOSERVICEWRAP_HH
#define	KARATHON_IOSERVICEWRAP_HH

#include <boost/python.hpp>
#include <karabo/net/IOService.hh>
#include "ScopedGILRelease.hh"
#include "ChannelWrap.hh"
#include "ConnectionWrap.hh"

namespace bp = boost::python;

namespace karathon {

    class IOServiceWrap {
    public:

        static void run(karabo::net::IOService::Pointer ioserv) {
            try {
                ScopedGILRelease nogil;
                ioserv->run();
                finalize(ioserv);
            } catch (const karabo::util::Exception& e) {
                finalize(ioserv);
                KARABO_RETHROW
            }
        }

        static void work(karabo::net::IOService::Pointer ioserv) {
            try {
                ScopedGILRelease nogil;
                ioserv->work();
                finalize(ioserv);
            } catch (const karabo::util::Exception& e) {
                finalize(ioserv);
                KARABO_RETHROW
            }
        }

        static void stop(karabo::net::IOService::Pointer ioserv) {
            try {
                ScopedGILRelease nogil;
                ioserv->stop();
            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW
            }
        }

        static void finalize(karabo::net::IOService::Pointer ioserv) {
            ChannelWrap::clear(ioserv);
            ConnectionWrap::clear(ioserv);
        }
    };
}

#endif	/* KARATHON_IOSERVICEWRAP_HH */

