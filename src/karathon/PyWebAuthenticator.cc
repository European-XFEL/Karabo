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

    bp::class_<Timestamp> ts("Timestamp");
    ts.def(bp::init<>());
    ts.def(bp::init<boost::posix_time::ptime const>(bp::arg("pt")));
    ts.def(bp::init<string const &>(bp::arg("timeStr")));
    ts.def("setMsSinceEpoch"
           , (void (Timestamp::*)(unsigned long long const))(&Timestamp::setMsSinceEpoch)
           , bp::arg("msSinceEpoch"));
    ts.def("setTime"
           , (void (Timestamp::*)(boost::posix_time::ptime const &))(&Timestamp::setTime)
           , bp::arg("timePoint"));
    ts.def("setTime"
           , (void (Timestamp::*)(std::string const &))(&Timestamp::setTime)
           , bp::arg("timePoint"));
    ts.def("getMsSinceEpoch"
           , (unsigned long long (Timestamp::*)() const) (&Timestamp::getMsSinceEpoch));
    ts.def("getTime"
           , (boost::posix_time::ptime(Timestamp::*)() const) (&Timestamp::getTime));


    bp::class_<Authenticator> a("Authenticator", bp::init<string const &, string const &, string const &, string const &, string const &, string const &, string const & >((bp::arg("username"), bp::arg("password"), bp::arg("provider"), bp::arg("ipAddress"), bp::arg("brokerHostname"), bp::arg("brokerPortNumber"), bp::arg("brokerTopic"))));
    a.def("login"
          , (bool (Authenticator::*)(Timestamp const &))(&Authenticator::login)
          , bp::arg("timestamp") = Timestamp());
    a.def("logout"
          , (bool (Authenticator::*)())(&Authenticator::logout));

    a.def("getSingleSignOn"
          , (string(Authenticator::*)(const string) const) (&Authenticator::getSingleSignOn)
          , bp::arg("ipAddress"));

    a.def("getSessionToken"
          , (string(Authenticator::*)() const) (&Authenticator::getSessionToken));

    a.def("getSoftware"
          , (string(Authenticator::*)() const) (&Authenticator::getSoftware));

    a.def("getBrokerPortNumber"
          , (string(Authenticator::*)() const) (&Authenticator::getBrokerPortNumber));

    a.def("getBrokerHostname"
          , (string(Authenticator::*)() const) (&Authenticator::getBrokerHostname));

    a.def("getBrokerTopic"
          , (string(Authenticator::*)() const) (&Authenticator::getBrokerTopic));

    a.def("getIpAddress"
          , (string(Authenticator::*)() const) (&Authenticator::getIpAddress));

    a.def("getProvider"
          , (string(Authenticator::*)() const) (&Authenticator::getProvider));

    a.def("getPassword"
          , (string(Authenticator::*)() const) (&Authenticator::getPassword));

    a.def("getUsername"
          , (string(Authenticator::*)() const) (&Authenticator::getUsername));

    a.def("getFirstName"
          , (string(Authenticator::*)() const) (&Authenticator::getFirstName));

    a.def("getFamilyName"
          , (string(Authenticator::*)() const) (&Authenticator::getFamilyName));

    a.def("getWelcomeMessage"
          , (string(Authenticator::*)() const) (&Authenticator::getWelcomeMessage));

    a.def("getDefaultAccessLevelId"
          , (long long (Authenticator::*)() const) (&Authenticator::getDefaultAccessLevelId));

    a.def("getDefaultAccessLevelDesc"
          , (string(Authenticator::*)() const) (&Authenticator::getDefaultAccessLevelDesc));

    a.def("getSoftwareId"
          , (long long (Authenticator::*)() const) (&Authenticator::getSoftwareId));

    a.def("getUserId"
          , (long long (Authenticator::*)() const) (&Authenticator::getUserId));
    
    a.def("getAccessHash"
          , (Hash (Authenticator::*)() const) (&Authenticator::getAccessHash));

}