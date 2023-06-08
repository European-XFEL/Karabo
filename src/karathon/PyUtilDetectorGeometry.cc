/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <boost/python.hpp>
#include <karabo/util/DetectorGeometry.hh>
#include <karabo/util/Hash.hh>
#include <karathon/Wrapper.hh>


// #include <karabo/util/ArrayTools.hh>


// using namespace std;
namespace bp = boost::python;
using namespace karabo::util;
using namespace karathon;

namespace karathon {


    struct DetectorGeometryWrap {
        static bp::object getOffsetsAsList(boost::shared_ptr<DetectorGeometry>& self) {
            return Wrapper::fromStdVectorToPyList(self->getOffsets());
        }


        static bp::object getRotationsAsList(boost::shared_ptr<DetectorGeometry>& self) {
            return Wrapper::fromStdVectorToPyList(self->getRotations());
        }
    };
} // namespace karathon


void exportPyUtilDetectorGeometry() {
    bp::class_<DetectorGeometry, boost::shared_ptr<DetectorGeometry> > d("DetectorGeometry");
    d.def(bp::init<>());
    d.def(bp::init<Hash>());
    d.def("getOffsets", &DetectorGeometryWrap::getOffsetsAsList);
    d.def("getRotations", &DetectorGeometryWrap::getRotationsAsList);
    d.def("toHash", &DetectorGeometry::toHash);
    d.def("toSchema", &DetectorGeometry::toSchema, (bp::arg("topNode"), bp::arg("schema"), bp::arg("topMost") = true));
    d.def("setOffsets", &DetectorGeometry::setOffsets, (bp::arg("x"), bp::arg("y"), bp::arg("z")),
          bp::return_internal_reference<>());
    d.def("setRotations", &DetectorGeometry::setRotations, (bp::arg("theta"), bp::arg("phi"), bp::arg("omega")),
          bp::return_internal_reference<>());
    d.def("startSubAssembly", &DetectorGeometry::startSubAssembly, bp::return_internal_reference<>());
    d.def("endSubAssembly", &DetectorGeometry::endSubAssembly, bp::return_internal_reference<>());
    d.def("setPixelRegion", &DetectorGeometry::setPixelRegion,
          (bp::arg("x0"), bp::arg("y0"), bp::arg("x1"), bp::arg("y1")), bp::return_internal_reference<>());
}
