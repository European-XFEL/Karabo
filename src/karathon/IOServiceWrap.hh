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

namespace bp = boost::python;

namespace karathon {

    class IOServiceWrap {

    public:

        static void run(karabo::net::IOService::Pointer ioserv) {
            ScopedGILRelease nogil;
            ioserv->run();
        }

        static void work(karabo::net::IOService::Pointer ioserv) {
            ScopedGILRelease nogil;
            ioserv->work();
        }

        static void stop(karabo::net::IOService::Pointer ioserv) {
            ScopedGILRelease nogil;
            ioserv->stop();
        }
    };
}

#endif	/* KARATHON_IOSERVICEWRAP_HH */

