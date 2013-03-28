/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/io/TextFileOutput.hh>
#include <karabo/io/FileTools.hh>
#include "PythonFactoryMacros.hh"

using namespace karabo::util;
using namespace karabo::io;
using namespace std;
namespace bp = boost::python;


void exportPyIoFileTools() {

    bp::def("saveSchemaToFile"
            , (void (*) (Schema const &, string const &, Hash const &))(&karabo::io::saveToFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("saveHashToFile"
            , (void (*) (Hash const &, string const &, Hash const &))(&karabo::io::saveToFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("loadHashFromFile"
            , (void (*) (Hash &, string const &, Hash const &))(&karabo::io::loadFromFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );
    
    {//exposing karabo::io::TextFileOutput<karabo::util::Hash>
        typedef karabo::io::TextFileOutput<karabo::util::Hash> WriterHash;
        bp::class_<WriterHash, boost::noncopyable >("WriterHash", bp::no_init)
                .def("write", &WriterHash::write)
                KARABO_PYTHON_FACTORY_CONFIGURATOR(WriterHash)
                ;
        bp::register_ptr_to_python< boost::shared_ptr<WriterHash> >();
    }

    {//exposing karabo::io::TextFileOutput<karabo::util::Schema>
        typedef karabo::io::TextFileOutput<karabo::util::Schema> WriterSchema;
        bp::class_<WriterSchema, boost::noncopyable > ("WriterSchema", bp::no_init)
                .def("write", &WriterSchema::write)
                KARABO_PYTHON_FACTORY_CONFIGURATOR(WriterSchema)
                ;
        bp::register_ptr_to_python< boost::shared_ptr<WriterSchema> >();
    }
}
