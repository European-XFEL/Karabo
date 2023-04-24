/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#include <boost/version.hpp>
#if BOOST_VERSION >= 107000
#include "BeastClientBase_1_81.hh"
#else
#include "BeastClientBase_1_68.hh"
#endif


namespace karabo {
    namespace net {


        int httpsPost(const std::string& host, const std::string& port, const std::string& target,
                      const std::string& body, int version, const std::function<void(bool, std::string)>& onComplete) {
            boost::asio::io_context ioc;
            // The SSL context is required, and holds certificates
#if BOOST_VERSION >= 107000
            ssl::context ctx{ssl::context::tlsv12_client};
#else
            ssl::context ctx{ssl::context::sslv23_client};
#endif

            // Verify the remote server's certificate (ssl::verify_peer)
            ctx.set_verify_mode(ssl::verify_none);

            // Launch the asynchronous operation
#if BOOST_VERSION >= 107000
            // The session is constructed with a strand to
            // ensure that handlers do not execute concurrently.
            std::make_shared<HttpsSession>(net::make_strand(ioc), ctx)
                  ->run(host, port, target, body, version, onComplete);
#else
            std::make_shared<HttpsSession>(ioc, ctx)->run(host, port, target, body, version, onComplete);
#endif

            // Run the I/O service. The call will return when
            // the get operation is complete.
            ioc.run();

            return EXIT_SUCCESS;
        }
    } // namespace net
} // namespace karabo
