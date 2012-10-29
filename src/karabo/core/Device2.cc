/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
//
//#include <boost/regex.hpp>
//#include <boost/algorithm/string.hpp>
//#include <string>
//#include <karabo/util/Time.hh>
//
//#include "Device2.hh"
//
//namespace karabo {
//    namespace core {
//
//        using namespace log4cpp;
//        using namespace std;
//        using namespace karabo::util;
//        using namespace karabo::io;
//        using namespace karabo::net;
//
//        
//        void Device2::errorFoundAction(const std::string& shortMessage, const std::string& detailedMessage) {
//            triggerErrorFound(shortMessage, detailedMessage);
//        }
//
//        void Device2::updateCurrentState(const std::string& state) {
//            set("state", state);
//            // Reply new state to interested event initiators
//            reply(state);
//        }
//        
//       
//     
//
//        void Device2::noStateTransition(const std::string& typeId, int state) {
//            string eventName(typeId);
//            boost::regex re(".*\\d+(.+Event).*");
//            boost::smatch what;
//            bool result = boost::regex_search(typeId, what, re);
//            if (result && what.size() == 2) {
//                eventName = what.str(1);
//            }
//            ostringstream msg;
//            msg << "Current state of device \"" << m_classId << "\" does not allow any transition for event \"" << eventName << "\"";
//            log() << Priority::DEBUG << msg.str();
//            emit("signalNoTransition", msg.str(), getInstanceId());
//        }
//    }
//}
