/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/io/TextFileOutput.hh>
#include <karabo/io/TextFileInput.hh>
#include <karabo/io/FileTools.hh>
#include "PythonFactoryMacros.hh"

using namespace karabo::util;
using namespace karabo::io;
using namespace std;
namespace bp = boost::python;


void exportPyIoFileTools() {

    bp::def("saveToFile"
            , (void (*) (Schema const &, string const &, Hash const &))(&karabo::io::saveToFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("saveToFile"
            , (void (*) (Hash const &, string const &, Hash const &))(&karabo::io::saveToFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("loadFromFile"
            , (void (*) (Hash &, string const &, Hash const &))(&karabo::io::loadFromFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );
    
    {//exposing karabo::io::TextFileOutput<karabo::util::Hash>
        typedef karabo::io::TextFileOutput<karabo::util::Hash> WriterHash;
        bp::class_<WriterHash, boost::noncopyable >("WriterHash", bp::no_init)
                .def("write"
                     , (void (WriterHash::*)(Hash const &))(&WriterHash::write)
                     , (bp::arg("data")))
                KARABO_PYTHON_FACTORY_CONFIGURATOR(WriterHash)
                ;
        bp::register_ptr_to_python< boost::shared_ptr<WriterHash> >();
    }
    
    {//exposing karabo::io::TextFileInput<karabo::util::Hash>
        typedef karabo::io::TextFileInput<karabo::util::Hash> ReaderHash;
        bp::class_<ReaderHash, boost::noncopyable >("ReaderHash", bp::no_init)
                .def("read"
                    , (void (ReaderHash::*)(Hash &, size_t))(&ReaderHash::read)
                    , (bp::arg("data"), bp::arg("idx")=0))
                .def("size" 
                     , (size_t (ReaderHash::*)() const)(&ReaderHash::size))
                KARABO_PYTHON_FACTORY_CONFIGURATOR(ReaderHash)
                ;
        bp::register_ptr_to_python< boost::shared_ptr<ReaderHash> >();
    }
    
    {//exposing karabo::io::TextFileOutput<karabo::util::Schema>
        typedef karabo::io::TextFileOutput<karabo::util::Schema> WriterSchema;
        bp::class_<WriterSchema, boost::noncopyable > ("WriterSchema", bp::no_init)
                .def("write"
                     , (void (WriterSchema::*)(Schema const &))(&WriterSchema::write)
                     , (bp::arg("data")))
                KARABO_PYTHON_FACTORY_CONFIGURATOR(WriterSchema)
                ;
        bp::register_ptr_to_python< boost::shared_ptr<WriterSchema> >();
    }
}
