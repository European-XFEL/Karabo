/*
  * $Id$
  *
  * Author: <irina.kozlova@xfel.eu>
  *
  * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
  */
#include <boost/python.hpp>

#include "ReconfigurableFsmWrap.hh"
#include <exfel/core/Device.hh>
#include <exfel/core/ReconfigurableFsm.hh>
#include <exfel/xms/SignalSlotable.hh>

using namespace std;
using namespace exfel::core;
using namespace exfel::util;
using namespace exfel::xms;
using namespace exfel::pyexfel;
namespace bp = boost::python;

void exportPyCoreReconfigurableFsm() {//exposing exfel::core::ReconfigurableFsm

    bp::class_<exfel::core::Device, bp::bases< exfel::xms::SignalSlotable>,  boost::noncopyable > ("Device", bp::no_init);
    
    bp::class_<ReconfigurableFsm, bp::bases< exfel::core::Device>, boost::noncopyable > ("ReconfigurableFsmIntern", bp::no_init);
    
     bp::class_<ReconfigurableFsmWrap, bp::bases< exfel::core::ReconfigurableFsm>, boost::noncopyable > ("ReconfigurableFsm", bp::no_init)
         .def("registerReconfigurableFsmDeviceClass",&ReconfigurableFsmWrap::registerReconfigurableFsmDeviceClass).staticmethod("registerReconfigurableFsmDeviceClass")
         .def("configure",  &ReconfigurableFsm::configure)
         .def("run",  &ReconfigurableFsm::run)
         .def("allOkStateOnEntry",  &ReconfigurableFsm::allOkStateOnEntry)
         .def("allOkStateOnExit",  &ReconfigurableFsm::allOkStateOnExit)
         .def("errorStateOnEntry",  &ReconfigurableFsm::errorStateOnEntry)
         .def("errorStateOnExit",  &ReconfigurableFsm::errorStateOnExit)
         ;
}
