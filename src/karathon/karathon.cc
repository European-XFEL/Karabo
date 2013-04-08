/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
//#include <karabo/util/ConfigConstants.hh>
//#include <karabo/xms/SignalSlotable.hh>
#include <boost/any.hpp>
#include <vector>

namespace bp = boost::python;

//void exportPyVectorContainer();
void exportPyUtilHash();
void exportPyUtilSchema();
//void exportPyUtilTypes();
void exportPyUtilClassInfo();
void exportPyIoFileTools();
//void exportPyIoReader();
//void exportPyIoFormat();
//void exportPyXmsRequestor();
//void exportPyXmsSignalSlotable();
//void exportChoiceElement();
//void exportSingleElement();
//void exportListElement();
//void exportSlotElement();
//void exportImageElement();
//void exportSimpleAnyElement();
//void exportComplexElement();
//void exportOverwriteElement();
//void exportNonEmptyListElement();
//void exportPyCoreDeviceClient();
//void exportTargetActualElement();
//void exportPyLogLogger();
void exportp2p();

//bp::object anyExtract(boost::any const& self) {
//    if (self.empty()) return bp::object();
//
//    KARABO_PYTHON_ANY_EXTRACT(std::string)
//    KARABO_PYTHON_ANY_EXTRACT(int)
//    KARABO_PYTHON_ANY_EXTRACT(unsigned int)
//    KARABO_PYTHON_ANY_EXTRACT(long long)
//    KARABO_PYTHON_ANY_EXTRACT(unsigned long long)
//    KARABO_PYTHON_ANY_EXTRACT(short)
//    KARABO_PYTHON_ANY_EXTRACT(unsigned short)
//    KARABO_PYTHON_ANY_EXTRACT(signed char)
//    KARABO_PYTHON_ANY_EXTRACT(unsigned char)
//    KARABO_PYTHON_ANY_EXTRACT(char)
//    KARABO_PYTHON_ANY_EXTRACT(double)
//    KARABO_PYTHON_ANY_EXTRACT(float)
//    KARABO_PYTHON_ANY_EXTRACT(karabo::util::Hash)
//
//    if (self.type() == typeid (bool)) {
//        return bp::object(boost::any_cast<bool > (self));
//    }
//    if (self.type() == typeid (std::deque<bool>)) {
//        typedef std::deque<bool> VContainer;
//        VContainer container(boost::any_cast<VContainer > (self));
//        std::string str;
//        str.append("[");
//        for (size_t i = 0; i < container.size(); i++) {
//            std::stringstream tmp;
//            tmp << container[i];
//            str.append(tmp.str());
//            if (i < container.size() - 1) str.append(",");
//        }
//        str.append("]");
//        return bp::object(str);
//    }
//
//    throw std::runtime_error("Unknown value type boost::any");
//}

BOOST_PYTHON_MODULE(libkarathon) {
    
//    bp::class_<boost::any > ("boost_any", bp::no_init)
//            .def("empty", &boost::any::empty)
//            .def("extract", anyExtract)
//            ;
//
//    bp::class_<std::vector<boost::any> >("stl_vector_boost_any", bp::no_init);
//
//    bp::class_<std::pair<const std::string, boost::any> >("hashPair")
//            .def_readonly("key", &std::pair<const std::string, boost::any>::first)
//            .def_readwrite("value", &std::pair<const std::string, boost::any>::second)
//            ;
//
//    bp::enum_<karabo::util::AccessType > ("AccessType")
//            .value("INIT", karabo::util::INIT)
//            .value("READ", karabo::util::READ)
//            .value("WRITE", karabo::util::WRITE)
//            .export_values()
//            ;
//
//    exportPyVectorContainer();
    exportPyUtilHash();
    exportPyUtilSchema();
//    exportPyUtilTypes();
    exportPyUtilClassInfo();
    exportPyIoFileTools();
//
//    exportPyXmsRequestor();
//    exportPyXmsSignalSlotable();
//    exportPyCoreDeviceClient();
//    

//    exportPyIoReader();
//    exportPyIoFormat();
//
//    exportChoiceElement();
//    exportSingleElement();
//    exportListElement();
//    exportSlotElement();
//    exportNonEmptyListElement();   
//    exportImageElement();
//    exportSimpleAnyElement();
//    exportComplexElement();
//    exportOverwriteElement();
//    exportTargetActualElement();
//    
//    exportPyLogLogger();
    exportp2p();
}
