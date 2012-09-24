#include <boost/python.hpp>
#include <boost/shared_ptr.hpp>

#include "Vehicle.hh"

using namespace std;
using namespace boost;
using namespace boost::python;
using namespace exfel::util;

boost::shared_ptr<Vehicle> (*create1)(const Hash&) = &Vehicle::create;

BOOST_PYTHON_MODULE(libvehiclepybind) {

    class_<Vehicle, boost::shared_ptr<Vehicle>, boost::noncopyable > ("Vehicle", no_init)
            .def("start", &Vehicle::start)
            .def("stop", &Vehicle::stop)
            .def("create", create1).staticmethod("create")
            ;
}
       
