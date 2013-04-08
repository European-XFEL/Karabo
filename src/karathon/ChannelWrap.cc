/* 
 * File:   ChannelWrap.cc
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 6, 2013, 12:47 AM
 */

#include "ChannelWrap.hh"
#include "ScopedGILAcquire.hh"

namespace bp = boost::python;
using namespace std;
using namespace karabo::util;
using namespace karabo::net;

std::map<karabo::net::Channel*, karabo::util::Hash> karabo::pyexfel::ChannelWrap::m_channelHandlers;
boost::mutex karabo::pyexfel::ChannelWrap::m_changedChannelHandlersMutex;

namespace karabo {
    namespace pyexfel {


        void ChannelWrap::read(Channel& channel, bp::object& obj) {
            if (PyByteArray_Check(obj.ptr())) {
                PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
                size_t size = PyByteArray_Size(bytearray);
                char* data = PyByteArray_AsString(bytearray);
                ScopedGILRelease nogil;
                channel.read(data, size);
                return;
            } else if (bp::extract<Hash&>(obj).check()) {
                Hash& hash = bp::extract<Hash&>(obj);
                ScopedGILRelease nogil;
                channel.read(hash);
            }
            throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
        }


        void ChannelWrap::read2(karabo::net::Channel& channel, bp::object& header, bp::object& obj) {
            if (bp::extract<karabo::util::Hash&>(header).check()) {
                karabo::util::Hash& hdr = bp::extract<karabo::util::Hash&>(header);
                if (PyByteArray_Check(obj.ptr())) {
                    PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
                    size_t size = PyByteArray_Size(bytearray);
                    char* data = PyByteArray_AsString(bytearray);
                    ScopedGILRelease nogil;
                    channel.read(hdr, data, size);
                    return;
                } else if (bp::extract<karabo::util::Hash&>(obj).check()) {
                    karabo::util::Hash& hash = bp::extract<karabo::util::Hash&>(obj);
                    ScopedGILRelease nogil;
                    channel.read(hdr, hash);
                }

            }
            throw KARABO_PYTHON_EXCEPTION("Python types in parameters are not supported");
        }


        void ChannelWrap::write(Channel& channel, const bp::object& obj) {
            if (PyByteArray_Check(obj.ptr())) {
                PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
                size_t size = PyByteArray_Size(bytearray);
                char* data = PyByteArray_AsString(bytearray);
                ScopedGILRelease nogil;
                channel.write(data, size);
                return;
            } else if (bp::extract<Hash&>(obj).check()) {
                const Hash& hash = bp::extract<const Hash&>(obj);
                ScopedGILRelease nogil;
                channel.write(hash);
            }
            throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
        }


        void ChannelWrap::write2(karabo::net::Channel& channel, const bp::object& header, const bp::object& obj) {
            if (bp::extract<karabo::util::Hash>(header).check()) {
                const karabo::util::Hash hdr = bp::extract<const karabo::util::Hash&>(header);
                if (PyByteArray_Check(obj.ptr())) {
                    PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
                    size_t size = PyByteArray_Size(bytearray);
                    char* data = PyByteArray_AsString(bytearray);
                    ScopedGILRelease nogil;
                    channel.write(hdr, data, size);
                    return;
                } else if (bp::extract<karabo::util::Hash&>(obj).check()) {
                    const karabo::util::Hash& hash = bp::extract<const karabo::util::Hash&>(obj);
                    ScopedGILRelease nogil;
                    channel.write(hdr, hash);
                }

            }
            throw KARABO_PYTHON_EXCEPTION("Python types in parameters are not supported");

        }


        void ChannelWrap::registerHandler(karabo::net::Channel& channel, const bp::object& handler) {
            karabo::util::Hash hash;
            // Check that 'handler' object is a class method
            if (hasattr(handler, "__self__")) { // class method
                const bp::object& selfObject = handler.attr("__self__");
                hash.set("_selfObject", selfObject.ptr());
                std::string funcName(bp::extract<std::string>(handler.attr("__name__")));
                hash.set("_function", funcName);
            } else { // free function
                hash.set("_function", handler.ptr());
            }
            {
                boost::mutex::scoped_lock lock(m_changedChannelHandlersMutex);
                m_channelHandlers[&channel] = hash;
            }
        }


        void ChannelWrap::readAsyncSizeInBytes(karabo::net::Channel& channel, const bp::object& handler) {
            registerHandler(channel, handler);
            channel.readAsyncSizeInBytes(proxyReadSizeInBytesHandler);
        }


        void ChannelWrap::proxyReadSizeInBytesHandler(karabo::net::Channel::Pointer channel, const size_t& size) {
            Hash hash;
            std::map<Channel*, Hash>::iterator it;
            {
                boost::mutex::scoped_lock lock(m_changedChannelHandlersMutex);
                it = m_channelHandlers.find(channel.get());
                if (it == m_channelHandlers.end())
                    throw KARABO_PYTHON_EXCEPTION("Logical error: connection is not registered");
                hash += it->second; // merge to hash
                m_channelHandlers.erase(it);
            }
            if (!hash.has("_selfObject") && !hash.has("_function"))
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");

            ScopedGILAcquire gil;
            if (hash.has("_selfObject") && hash.has("_function")) {
                // Class method is handler
                PyObject* objptr = hash.get<PyObject*>("_selfObject");
                std::string funcName = hash.get<std::string>("_function");
                bp::call_method<void>(objptr, funcName.c_str(), bp::object(channel), bp::object(size));
            } else if (hash.has("_function")) {
                // Free function is handler
                PyObject* funcptr = hash.get<PyObject*>("_function");
                bp::call<void>(funcptr, bp::object(channel), bp::object(size));
            } else
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters do not fit!");
        }


        void ChannelWrap::readAsyncStr(karabo::net::Channel& channel, bp::object& obj, const bp::object& handler) {
            registerHandler(channel, handler);
            if (!PyByteArray_Check(obj.ptr()))
                throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
            PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
            size_t size = PyByteArray_Size(bytearray);
            char* data = PyByteArray_AsString(bytearray);
            channel.readAsyncRaw(data, size, proxyReadRawHandler);
        }


        void ChannelWrap::proxyReadRawHandler(karabo::net::Channel::Pointer channel) {
            Hash hash;
            std::map<Channel*, Hash>::iterator it;
            {
                boost::mutex::scoped_lock lock(m_changedChannelHandlersMutex);
                it = m_channelHandlers.find(channel.get());
                if (it == m_channelHandlers.end())
                    throw KARABO_PYTHON_EXCEPTION("Logical error: connection is not registered");
                hash += it->second; // merge to hash
                m_channelHandlers.erase(it);
            }
            if (!hash.has("_selfObject") && !hash.has("_function"))
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");

            ScopedGILAcquire gil;
            if (hash.has("_selfObject") && hash.has("_function")) {
                // Class method is handler
                PyObject* objptr = hash.get<PyObject*>("_selfObject");
                std::string funcName = hash.get<std::string>("_function");
                bp::call_method<void>(objptr, funcName.c_str(), bp::object(channel));
            } else if (hash.has("_function")) {
                // Free function is handler
                PyObject* funcptr = hash.get<PyObject*>("_function");
                bp::call<void>(funcptr, bp::object(channel));
            } else
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters do not fit!");
        }


        void ChannelWrap::readAsyncHash(karabo::net::Channel& channel, const bp::object& handler) {
            registerHandler(channel, handler);
            channel.readAsyncHash(proxyReadHashHandler);
        }


        void ChannelWrap::proxyReadHashHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash& h) {
            Hash hash;
            std::map<Channel*, Hash>::iterator it;
            {
                boost::mutex::scoped_lock lock(m_changedChannelHandlersMutex);
                it = m_channelHandlers.find(channel.get());
                if (it == m_channelHandlers.end())
                    throw KARABO_PYTHON_EXCEPTION("Logical error: connection is not registered");
                hash += it->second; // merge to hash
                m_channelHandlers.erase(it);
            }
            if (!hash.has("_selfObject") && !hash.has("_function"))
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");

            ScopedGILAcquire gil;
            if (hash.has("_selfObject") && hash.has("_function")) {
                // Class method is handler
                PyObject* objptr = hash.get<PyObject*>("_selfObject");
                std::string funcName = hash.get<std::string>("_function");
                bp::call_method<void>(objptr, funcName.c_str(), bp::object(channel), bp::object(h));
            } else if (hash.has("_function")) {
                // Free function is handler
                PyObject* funcptr = hash.get<PyObject*>("_function");
                bp::call<void>(funcptr, bp::object(channel), bp::object(h));
            } else
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters do not fit!");
        }


        void ChannelWrap::readAsyncHashStr(karabo::net::Channel& channel, const bp::object& handler) {
            registerHandler(channel, handler);
            channel.readAsyncHashVector(proxyReadHashVectorHandler);
        }


        void ChannelWrap::proxyReadHashVectorHandler(karabo::net::Channel::Pointer channel,
                                        const karabo::util::Hash& h, const std::vector<char>& v) {
            Hash hash;
            std::map<Channel*, Hash>::iterator it;
            {
                boost::mutex::scoped_lock lock(m_changedChannelHandlersMutex);
                it = m_channelHandlers.find(channel.get());
                if (it == m_channelHandlers.end())
                    throw KARABO_PYTHON_EXCEPTION("Logical error: connection is not registered");
                hash += it->second; // merge to hash
                m_channelHandlers.erase(it);
            }
            if (!hash.has("_selfObject") && !hash.has("_function"))
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");

            ScopedGILAcquire gil;
            if (hash.has("_selfObject") && hash.has("_function")) {
                // Class method is handler
                PyObject* objptr = hash.get<PyObject*>("_selfObject");
                std::string funcName = hash.get<std::string>("_function");
                bp::call_method<void>(objptr, funcName.c_str(), bp::object(channel), bp::object(h), bp::object(v));
            } else if (hash.has("_function")) {
                // Free function is handler
                PyObject* funcptr = hash.get<PyObject*>("_function");
                bp::call<void>(funcptr, bp::object(channel), bp::object(h), bp::object(v));
            } else
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters do not fit!");
        }


        void ChannelWrap::readAsyncHashHash(karabo::net::Channel& channel, const bp::object& handler) {

        }


        void ChannelWrap::writeAsyncStr(karabo::net::Channel& channel, const bp::object& data, const bp::object& handler) {

        }


        void ChannelWrap::writeAsyncHash(karabo::net::Channel& channel, const bp::object& data, const bp::object& handler) {

        }


        void ChannelWrap::writeAsyncHashStr(karabo::net::Channel& channel, const bp::object& hdr, const bp::object& data, const bp::object& handler) {

        }


        void ChannelWrap::writeAsyncHashHash(karabo::net::Channel& channel, const bp::object& hdr, const bp::object& data, const bp::object& handler) {

        }
    }
}