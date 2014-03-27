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

    bp::class_<Authenticator> a("Authenticator", bp::init<string const &, string const &, string const &, string const &, string const &, int const, string const & >((bp::arg("username"), bp::arg("password"), bp::arg("provider"), bp::arg("ipAddress"), bp::arg("brokerHostname"), bp::arg("brokerPortNumber"), bp::arg("brokerTopic"))));

    a.def("login"
          , (bool (Authenticator::*)())(&Authenticator::login));
    a.def("logout"
          , (bool (Authenticator::*)())(&Authenticator::logout));

    a.def("getSingleSignOn"
          , (string(Authenticator::*)(const string) const) (&Authenticator::getSingleSignOn)
          , bp::arg("ipAddress"));

    a.def("getSessionToken"
          , (string const & (Authenticator::*)() const) (&Authenticator::getSessionToken)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getSoftware"
          , (string const & (Authenticator::*)() const) (&Authenticator::getSoftware)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getBrokerPortNumber"
          , (int const (Authenticator::*)() const) (&Authenticator::getBrokerPortNumber));

    a.def("getBrokerHostname"
          , (string const & (Authenticator::*)() const) (&Authenticator::getBrokerHostname)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getBrokerTopic"
          , (string const & (Authenticator::*)() const) (&Authenticator::getBrokerTopic)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getIpAddress"
          , (string const & (Authenticator::*)() const) (&Authenticator::getIpAddress)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getProvider"
          , (string const & (Authenticator::*)() const) (&Authenticator::getProvider)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getPassword"
          , (string const & (Authenticator::*)() const) (&Authenticator::getPassword)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getUsername"
          , (string const & (Authenticator::*)() const) (&Authenticator::getUsername)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getFirstName"
          , (string const & (Authenticator::*)() const) (&Authenticator::getFirstName)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getFamilyName"
          , (string const & (Authenticator::*)() const) (&Authenticator::getFamilyName)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getWelcomeMessage"
          , (string const & (Authenticator::*)() const) (&Authenticator::getWelcomeMessage)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getDefaultAccessLevelId"
          , (int (Authenticator::*)() const) (&Authenticator::getDefaultAccessLevelId));

    a.def("getDefaultAccessLevelDesc"
          , (string const & (Authenticator::*)() const) (&Authenticator::getDefaultAccessLevelDesc)
          , bp::return_value_policy<bp::copy_const_reference > ());

    a.def("getSoftwareId"
          , (long long (Authenticator::*)() const) (&Authenticator::getSoftwareId));

    a.def("getUserId"
          , (long long (Authenticator::*)() const) (&Authenticator::getUserId));

    a.def("getAccessHash"
          , (Hash(Authenticator::*)() const) (&Authenticator::getAccessHash));

}