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
    
    bp::class_<Authenticator> a( "Authenticator", bp::init<string const &, string const &, string const &, string const &, string const &, string const &, string const & >(( bp::arg("username"), bp::arg("password"), bp::arg("provider"), bp::arg("ipAddress"), bp::arg("hostname"), bp::arg("portNumber"), bp::arg("software") )) );
        a.def("login"
            , (bool (Authenticator::*)(Timestamp const &))(&Authenticator::login)
            , bp::arg("timestamp") = Timestamp());    
        a.def("logout"
            , (bool (Authenticator::*)())(&Authenticator::logout));
        
        a.def("getSingleSignOn"
            , (string (Authenticator::*)(const string) const)(&Authenticator::getSingleSignOn)
            , bp::arg("ipAddress"));

       a.def("getSessionToken"
            , (string (Authenticator::*)() const)(&Authenticator::getSessionToken));
     
       a.def("getSoftware"
            , (string (Authenticator::*)() const)(&Authenticator::getSoftware));
       
       a.def("getPortNumber"
           , (string (Authenticator::*)() const)(&Authenticator::getPortNumber));
       
       a.def("getHostname"
           , (string (Authenticator::*)() const)(&Authenticator::getHostname));
       
       a.def("getIpAddress"
           , (string (Authenticator::*)() const)(&Authenticator::getIpAddress));
       
       a.def("getProvider"
           , (string (Authenticator::*)() const)(&Authenticator::getProvider));
       
       a.def("getPassword"
           , (string (Authenticator::*)() const)(&Authenticator::getPassword));

       a.def("getUsername"
           , (string (Authenticator::*)() const)(&Authenticator::getUsername));
       
       a.def("getRoleDesc"
           , (string (Authenticator::*)() const)(&Authenticator::getRoleDesc));
       
       a.def("getWelcomeMessage"
           , (string (Authenticator::*)() const)(&Authenticator::getWelcomeMessage));
       
       a.def("getRoleId"
           , (unsigned long long (Authenticator::*)() const)(&Authenticator::getRoleId));
       
       a.def("getSoftwareId"
           , (unsigned long long (Authenticator::*)() const)(&Authenticator::getSoftwareId));

       a.def("getUserId"
           , (unsigned long long (Authenticator::*)() const)(&Authenticator::getUserId));

}