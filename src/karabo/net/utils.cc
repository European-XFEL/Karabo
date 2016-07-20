/*
 * $Id$
 *
 * Author: gero.flucke@xfel.eu>
 *
 * Created on February 11, 2016
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>

#include <boost/asio.hpp>
#include <boost/regex.hpp>

#include "karabo/net/utils.hh"
#include "karabo/util/Exception.hh"
#include "karabo/util/StringTools.hh"
#include "karabo/log/Logger.hh"

using namespace std;
using namespace boost;


std::string karabo::net::bareHostName() {
    std::string hostName = boost::asio::ip::host_name();

    // Find first dot and erase everything after it:
    const std::string::size_type dotPos = hostName.find('.');
    if (dotPos != std::string::npos) hostName.erase(dotPos);

    return hostName;
}


void karabo::net::runProtected(boost::shared_ptr<boost::asio::io_service> service,
                               const std::string& category, const std::string& errorMessage,
                               unsigned int delayInMilliSec) {
    // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference/io_service.html:
    // "If an exception is thrown from a handler, the exception is allowed to propagate through the throwing
    //  thread's invocation of run(), run_one(), poll() or poll_one(). No other threads that are calling any of
    //  these functions are affected. It is then the responsibility of the application to catch the exception.
    //
    //  After the exception has been caught, the run(), run_one(), poll() or poll_one() call may be restarted
    //  without the need for an intervening call to reset(). This allows the thread to rejoin the io_service
    //  object's thread pool without impacting any other threads in the pool."

    const std::string fullMessage(" when running asio service (" + errorMessage + "), continue in " +
                                  karabo::util::toString(delayInMilliSec) += " ms");
    while (true) {
        bool caught = true;
        try {
            service->run();
            caught = false; // run exited normally
            break;
        } catch (karabo::util::Exception& e) {
            KARABO_LOG_FRAMEWORK_ERROR_C(category) << "Exception" << fullMessage << ": " << e;
        } catch (std::exception& e) {
            KARABO_LOG_FRAMEWORK_ERROR_C(category) << "Standard exception" << fullMessage << ": " << e.what();
        } catch (...) {
            KARABO_LOG_FRAMEWORK_ERROR_C(category) << "Unknown exception" << fullMessage << ".";
        }
        if (caught) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(delayInMilliSec));
        }
    }
}


boost::tuple<std::string, std::string, std::string, std::string, std::string> karabo::net::parseUrl(const std::string& url) {
    boost::regex ex("(.+)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
    boost::cmatch what;
    if (regex_match(url.c_str(), what, ex)) {
        string protocol = string(what[1].first, what[1].second);
        string domain = string(what[2].first, what[2].second);
        string port = string(what[3].first, what[3].second);
        string path = string(what[4].first, what[4].second);
        string query = string(what[5].first, what[5].second);
        return make_tuple(protocol, domain, port, path, query);
    }
}
