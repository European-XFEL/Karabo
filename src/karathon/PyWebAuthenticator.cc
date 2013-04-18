/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>
#undef HAVE_POLL
#undef HAVE_SNPRINTF
#undef HAVE_FTIME
#undef HAVE_TIMEGM
#include <karabo/webAuth/Authenticator.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace karabo::webAuth;
using namespace std;

void exportPyWebAuthenticator() {
    
    bp::class_<karabo::util::Timestamp>("Timestamp", bp::init<>());
    
    bp::class_<Authenticator>( "Authenticator", bp::init<string const &, string const &, string const &, string const &, string const &, string const &, string const & >(( bp::arg("username"), bp::arg("password"), bp::arg("provider"), bp::arg("ipAddress"), bp::arg("hostname"), bp::arg("portNumber"), bp::arg("software") )) )
        .def("login"
            , (bool (Authenticator::*)(Timestamp const &))(&Authenticator::login)
            , bp::arg("timestamp") = Timestamp())    
        .def("logout"
            , (bool (Authenticator::*)())(&Authenticator::logout))
        ;
}
