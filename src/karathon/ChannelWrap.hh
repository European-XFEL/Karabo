/* 
 * File:   ChannelWrap.hh
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 4, 2013, 3:22 PM
 */

#ifndef KARABO_PYEXFEL_CHANNELWRAP_HH
#define	KARABO_PYEXFEL_CHANNELWRAP_HH

#include <boost/python.hpp>
#include <karabo/net/Channel.hh>
#include "ScopedGILRelease.hh"

namespace bp = boost::python;

namespace karabo {
    namespace pyexfel {


        class ChannelWrap {
        public:

            static bp::object getConnection(karabo::net::Channel& channel) {
                return bp::object(channel.getConnection());
            }

            static size_t readSizeInBytes(karabo::net::Channel& channel) {
                ScopedGILRelease gil;
                return channel.readSizeInBytes();
            }

            static void read(karabo::net::Channel& channel, bp::object& obj) {
                if (!PyByteArray_Check(obj.ptr()))
                    throw KARABO_PYTHON_EXCEPTION("Pythob object in parameters is not a python bytearray");
                PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
                size_t size = PyByteArray_Size(bytearray);
                char* data = PyByteArray_AsString(bytearray);
                ScopedGILRelease gil;
                channel.read(data, size);
            }
        };
    }
}
#endif	/* KARABO_PYEXFEL_CHANNELWRAP_HH */

