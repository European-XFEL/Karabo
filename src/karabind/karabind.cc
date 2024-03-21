/*
 * Author: CONTROLS DEV group
 *
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

#include <pybind11/pybind11.h>

namespace py = pybind11;

// util
void exportPyUtilClassInfo(py::module_&);             // PyUtilClassInfo.cc
void exportPyUtilTypesReferenceType(py::module_&);    // PyUtilTypesReferenceType.cc
void exportPyUtilTimestamp(py::module_&);             // PyUtilTimestamp.cc
void exportPyUtilTimeDuration(py::module_&);          // PyUtilTimeDuration.cc
void exportPyUtilTrainstamp(py::module_&);            // PyUtilTrainstamp.cc
void exportPyUtilHashAttributes(py::module_&);        // PyUtilHashAttributes.cc
void exportPyUtilDims(py::module_&);                  // PyUtilDims.cc
void exportPyUtilHashNode(py::module_&);              // PyUtilHashNode.cc
void exportPyUtilNDArray(py::module_&);               // PyUtilNDArray.cc
void exportPyUtilHash(py::module_&);                  // PyUtilHash.cc
void exportPyUtilAlarmConditionElement(py::module_&); // PyUtilAlarmElement.cc
void exportPyUtilStateElement(py::module_&);          // PyUtilStateElement.cc
void exportPyUtilDateTimeString(py::module_&);        // PyUtilDateTimeString.cc
void exportPyUtilEpochstamp(py::module_&);            // PyUtilEpochstamp.cc
void exportPyUtilException(py::module_&);             // PyUtilException.cc
void exportPyUtilRollingWindowStatistics(py::module_&);
void exportPyUtilSchema(py::module_&);                 // PyUtilSchema.cc
void exportPyUtilSchemaElement(py::module_&);          // PyUtilSchemaElement.cc (simple & vector)
void exportPyUtilSchemaNodeElement(py::module_&);      // PyUtilSchemaNodeElement.cc
void exportPyUtilSchemaChoiceElement(py::module_&);    // PyUtilSchemaChoiceElement.cc
void exportPyUtilSchemaListElement(py::module_&);      // PyUtilSchemaListElement.cc
void exportPyUtilSchemaOverwriteElement(py::module_&); // PyUtilSchemaOverwriteElement.cc
void exportPyUtilSchemaTableElement(py::module_&);     // PyUtilSchemaTableElement.cc
void exportPyUtilSchemaValidator(py::module_&);        // PyUtilSchemaValidator.cc
void exportPyUtilJsonToHashParser(py::module_&);       // PyUtilJsonToHashParser.cc

// io
void exportPyIoFileToolsAll(py::module_&);

// xms
void exportPyXmsImageDataElement(py::module_&);   // PyXmsImageDataElement.cc
void exportPyXmsSignalSlotable(py::module_&);     // PyXmsSignalSlotable.cc
void exportPyXmsSlotElement(py::module_&);        // PyXmsSlotElement.cc
void exportPyXmsInputOutputChannel(py::module_&); // PyXmsInputOutputChannel.cc

// core
void exportPyCoreDeviceClient(py::module_&); // PyCoreDeviceClient.cc
void exportPyCoreLock(py::module_&);         // PyCoreLockWrap.cc

// log
void exportPyLogLogger(py::module_&); // PyLogLogger.cc

// net
void exportPyNetEventLoop(py::module_&);         // PyNetEventLoop.cc
void exportPyNetConnectionChannel(py::module_&); // PyNetConnectionChannel.cc

// utilities
void exportPyKarabindTestUtilities(py::module_&); // ConfigurationTestClasses.[cc,hh]

// Build one big module, 'karabind.so', similar to how we build 'karathon' module

PYBIND11_MODULE(karabind, m) {
    // util
    exportPyUtilClassInfo(m);
    exportPyUtilTypesReferenceType(m);
    exportPyUtilHashAttributes(m);
    exportPyUtilHashNode(m);
    exportPyUtilNDArray(m);
    exportPyUtilHash(m);
    exportPyUtilEpochstamp(m);
    exportPyUtilException(m);
    exportPyUtilTimestamp(m);
    exportPyUtilTimeDuration(m);
    exportPyUtilTrainstamp(m);
    exportPyUtilDateTimeString(m);
    exportPyUtilDims(m);
    exportPyUtilAlarmConditionElement(m);
    exportPyUtilStateElement(m);
    exportPyUtilRollingWindowStatistics(m);
    exportPyUtilSchema(m);
    exportPyUtilSchemaElement(m);
    exportPyUtilSchemaNodeElement(m);
    exportPyUtilSchemaChoiceElement(m);
    exportPyUtilSchemaListElement(m);
    exportPyUtilSchemaOverwriteElement(m);
    exportPyUtilSchemaTableElement(m);
    exportPyUtilSchemaValidator(m);
    exportPyUtilJsonToHashParser(m);

    // io
    exportPyIoFileToolsAll(m);

    // xms
    exportPyXmsImageDataElement(m);
    exportPyXmsSignalSlotable(m);
    exportPyXmsSlotElement(m);
    exportPyXmsInputOutputChannel(m);

    // core
    exportPyCoreDeviceClient(m);
    exportPyCoreLock(m);

    // log
    exportPyLogLogger(m);

    // net
    exportPyNetEventLoop(m);
    exportPyNetConnectionChannel(m);

    // exportPyKarabindTestUtilities
    exportPyKarabindTestUtilities(m);
}
