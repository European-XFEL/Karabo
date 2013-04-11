/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Device.hh"
#include "OkErrorFsm.hh"

namespace karabo {
    namespace core {

        template class Device<>;
       

//        void Device::triggerErrorFound(const std::string& shortMessage, const std::string& detailedMessage) const {
//            emit("signalErrorFound", karabo::util::Time::getCurrentDateTime(), shortMessage, detailedMessage, getInstanceId());
//        }
//
//        void Device::triggerWarning(const std::string& warningMessage) const {
//            emit("signalWarning", karabo::util::Time::getCurrentDateTime(), warningMessage, getInstanceId());
//        }
//
//        void Device::triggerAlarm(const std::string& alarmMessage) const {
//            emit("signalAlarm", karabo::util::Time::getCurrentDateTime(), alarmMessage, getInstanceId());
//        }
    }
}
