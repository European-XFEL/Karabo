/*
 * File:   AsioIOService.cc
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 12:21 PM
 */

#include "AsioIOService.hh"
#include "karabo/log/Logger.hh"

namespace karabo {
    namespace net {
        KARABO_REGISTER_IN_FACTORY(AbstractIOService, AsioIOService)

        void AsioIOService::runProtected() {
            // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference/io_service.html:
            // "If an exception is thrown from a handler, the exception is allowed to propagate through the throwing
            //  thread's invocation of run(), run_one(), poll() or poll_one(). No other threads that are calling any of
            //  these functions are affected. It is then the responsibility of the application to catch the exception.
            //
            //  After the exception has been caught, the run(), run_one(), poll() or poll_one() call may be restarted
            //  without the need for an intervening call to reset(). This allows the thread to rejoin the io_service
            //  object's thread pool without impacting any other threads in the pool."
            while (true) {
                try {
                    m_ioservice->run();
                    break; // run exited normally
                } catch(karabo::util::Exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Exception when running io service: " << e;
                } catch(std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception when running io service: " << e.what();
                } catch(...) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception when running io service";
                }
            }
        }
    }
}
