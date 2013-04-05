/* 
 * File:   p2pbinding.cc
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 4, 2013, 4:04 PM
 */

#include <boost/python.hpp>
#include <karabo/net/IOService.hh>
#include <karabo/net/Connection.hh>
#include "PythonFactoryMacros.hh"
#include "IOServiceWrap.hh"
#include "ConnectionWrap.hh"
#include "ChannelWrap.hh"

namespace bp = boost::python;

using namespace std;
using namespace karabo::net;
using namespace karabo::pyexfel;

void exportp2p() {
    bp::docstring_options docs(true, true, false);
    
    {
        bp::class_<ErrorCode> ec("ErrorCode", bp::init<>());
    }

    {
        bp::class_<IOService, boost::shared_ptr<IOService> >("IOService", bp::no_init)
                .def("run", &IOServiceWrap().run)
                .def("work", &IOServiceWrap().work)
                .def("stop", &IOServiceWrap().stop)
                ;
    }
    {
        bp::class_<Connection, Connection::Pointer, boost::noncopyable>("Connection", bp::no_init)
                .def("expectedParameters", &Connection::expectedParameters, (bp::arg("expected")))
                .def("start", &ConnectionWrap().start)
                .def("startAsync", &ConnectionWrap().startAsync, (bp::arg("handler")))
                .def("stop", &ConnectionWrap().stop)
                .def("createChannel", &Connection::createChannel)
                .def("getIOService", &Connection::getIOService)
                .def("setIOService", &ConnectionWrap().setIOService, (bp::arg("ioserv")))
                KARABO_PYTHON_FACTORY_CONFIGURATOR(Connection)
                ;
    }
    {
        bp::class_<Channel, Channel::Pointer, boost::noncopyable >("Channel", bp::no_init)
                .def("getConnection", &Channel::getConnection)
                .def("readSizeInBytes", &ChannelWrap().readSizeInBytes)
                KARABO_PYTHON_FACTORY_CONFIGURATOR(Channel)
                ;
    }
}