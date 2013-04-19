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


        #define CALL_PYTHON_HANDLER_WITH_0()             Hash hash;\
            std::map<Channel*, Hash>::iterator it;\
            {\
                boost::mutex::scoped_lock lock(m_changedChannelHandlersMutex);\
                it = m_channelHandlers.find(channel.get());\
                if (it == m_channelHandlers.end())\
                    throw KARABO_PYTHON_EXCEPTION("Logical error: connection is not registered");\
                hash += it->second;\
                m_channelHandlers.erase(it);\
            }\
            if (!hash.has("_selfObject") && !hash.has("_function"))\
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");\
            ScopedGILAcquire gil;\
            if (hash.has("_selfObject") && hash.has("_function")) {\
                PyObject* objptr = hash.get<PyObject*>("_selfObject");\
                std::string funcName = hash.get<std::string>("_function");\
                bp::call_method<void>(objptr, funcName.c_str(), bp::object(channel));\
            } else if (hash.has("_function")) {\
                PyObject* funcptr = hash.get<PyObject*>("_function");\
                bp::call<void>(funcptr, bp::object(channel));\
            } else\
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters do not fit!")


        #define CALL_PYTHON_HANDLER_WITH_1(x)             Hash hash;\
            std::map<Channel*, Hash>::iterator it;\
            {\
                boost::mutex::scoped_lock lock(m_changedChannelHandlersMutex);\
                it = m_channelHandlers.find(channel.get());\
                if (it == m_channelHandlers.end())\
                    throw KARABO_PYTHON_EXCEPTION("Logical error: connection is not registered");\
                hash += it->second;\
                m_channelHandlers.erase(it);\
            }\
            if (!hash.has("_selfObject") && !hash.has("_function"))\
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");\
            ScopedGILAcquire gil;\
            if (hash.has("_selfObject") && hash.has("_function")) {\
                PyObject* objptr = hash.get<PyObject*>("_selfObject");\
                std::string funcName = hash.get<std::string>("_function");\
                bp::call_method<void>(objptr, funcName.c_str(), bp::object(channel), x);\
            } else if (hash.has("_function")) {\
                PyObject* funcptr = hash.get<PyObject*>("_function");\
                bp::call<void>(funcptr, bp::object(channel), x);\
            } else\
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters do not fit!")


        #define CALL_PYTHON_HANDLER_WITH_2(x,y)             Hash hash;\
            std::map<Channel*, Hash>::iterator it;\
            {\
                boost::mutex::scoped_lock lock(m_changedChannelHandlersMutex);\
                it = m_channelHandlers.find(channel.get());\
                if (it == m_channelHandlers.end())\
                    throw KARABO_PYTHON_EXCEPTION("Logical error: connection is not registered");\
                hash += it->second;\
                m_channelHandlers.erase(it);\
            }\
            if (!hash.has("_selfObject") && !hash.has("_function"))\
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");\
            ScopedGILAcquire gil;\
            if (hash.has("_selfObject") && hash.has("_function")) {\
                PyObject* objptr = hash.get<PyObject*>("_selfObject");\
                std::string funcName = hash.get<std::string>("_function");\
                bp::call_method<void>(objptr, funcName.c_str(), bp::object(channel), x, y);\
            } else if (hash.has("_function")) {\
                PyObject* funcptr = hash.get<PyObject*>("_function");\
                bp::call<void>(funcptr, bp::object(channel), x, y);\
            } else\
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters do not fit!")


        #define CALL_PYTHON_ERROR_HANDLER_WITH_1(x)             Hash hash;\
            std::map<Channel*, Hash>::iterator it;\
            {\
                boost::mutex::scoped_lock lock(m_changedChannelHandlersMutex);\
                it = m_channelHandlers.find(channel.get());\
                if (it == m_channelHandlers.end())\
                    throw KARABO_PYTHON_EXCEPTION("Logical error: connection is not registered");\
                hash += it->second;\
            }\
            if (!hash.has("_selfObject") && !hash.has("_function"))\
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");\
            ScopedGILAcquire gil;\
            if (hash.has("_selfObject") && hash.has("_function")) {\
                PyObject* objptr = hash.get<PyObject*>("_selfObject");\
                std::string funcName = hash.get<std::string>("_function");\
                bp::call_method<void>(objptr, funcName.c_str(), bp::object(channel), x);\
            } else if (hash.has("_function")) {\
                PyObject* funcptr = hash.get<PyObject*>("_function");\
                bp::call<void>(funcptr, bp::object(channel), x);\
            } else\
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters do not fit!")


        void ChannelWrap::read(Channel& channel, bp::object& obj) {
            if (PyByteArray_Check(obj.ptr())) {
                PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
                size_t size = PyByteArray_Size(bytearray);
                char* data = PyByteArray_AsString(bytearray);
                ScopedGILRelease nogil;
                channel.read(data, size);
                return;
            } else if (bp::extract<karabo::util::Hash&>(obj).check()) {
                karabo::util::Hash& hash = bp::extract<karabo::util::Hash&>(obj);
                ScopedGILRelease nogil;
                channel.read(hash);
                return;
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
                    return;
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
                return;
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
                    return;
                }

            }
            throw KARABO_PYTHON_EXCEPTION("Python types in parameters are not supported");

        }


        void ChannelWrap::readAsyncSizeInBytes(karabo::net::Channel& channel, const bp::object& handler) {
            registerHandler(channel, handler);
            channel.readAsyncSizeInBytes(proxyReadSizeInBytesHandler);
        }


        void ChannelWrap::proxyReadSizeInBytesHandler(karabo::net::Channel::Pointer channel, const size_t& size) {
            CALL_PYTHON_HANDLER_WITH_1(bp::object(size));
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
            CALL_PYTHON_HANDLER_WITH_0();
        }


        void ChannelWrap::readAsyncHash(karabo::net::Channel& channel, const bp::object& handler) {
            registerHandler(channel, handler);
            channel.readAsyncHash(proxyReadHashHandler);
        }


        void ChannelWrap::proxyReadHashHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash& h) {
            CALL_PYTHON_HANDLER_WITH_1(bp::object(h));
        }


        void ChannelWrap::readAsyncHashStr(karabo::net::Channel& channel, const bp::object& handler) {
            registerHandler(channel, handler);
            channel.readAsyncHashVector(proxyReadHashVectorHandler);
        }


        void ChannelWrap::proxyReadHashVectorHandler(karabo::net::Channel::Pointer channel,
                                                     const karabo::util::Hash& h, const std::vector<char>& v) {
            CALL_PYTHON_HANDLER_WITH_2(bp::object(h), bp::object(v));
        }


        void ChannelWrap::readAsyncHashHash(karabo::net::Channel& channel, const bp::object& handler) {
            registerHandler(channel, handler);
            channel.readAsyncHashHash(proxyReadHashHashHandler);
        }


        void ChannelWrap::proxyReadHashHashHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash& h, const karabo::util::Hash& b) {
            CALL_PYTHON_HANDLER_WITH_2(bp::object(h), bp::object(b));
        }


        void ChannelWrap::writeAsyncStr(karabo::net::Channel& channel, const bp::object& obj, const bp::object& handler) {
            if (PyByteArray_Check(obj.ptr())) {
                PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
                size_t size = PyByteArray_Size(bytearray);
                char* data = PyByteArray_AsString(bytearray);
                registerHandler(channel, handler);
                channel.writeAsyncRaw(data, size, proxyWriteCompleteHandler);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
        }


        void ChannelWrap::proxyWriteCompleteHandler(karabo::net::Channel::Pointer channel) {
            CALL_PYTHON_HANDLER_WITH_0();
        }


        void ChannelWrap::writeAsyncHash(karabo::net::Channel& channel, const bp::object& data, const bp::object& handler) {
            if (bp::extract<Hash>(data).check()) {
                const Hash& h = bp::extract<Hash>(data);
                registerHandler(channel, handler);
                channel.writeAsyncHash(h, proxyWriteCompleteHandler);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
        }


        void ChannelWrap::writeAsyncHashStr(karabo::net::Channel& channel, const bp::object& hdr, const bp::object& body, const bp::object& handler) {
            if (bp::extract<Hash>(hdr).check() && PyByteArray_Check(body.ptr())) {
                const Hash& h = bp::extract<Hash>(hdr);
                PyObject* bytearray = PyByteArray_FromObject(body.ptr());
                size_t size = PyByteArray_Size(bytearray);
                char* data = PyByteArray_AsString(bytearray);
                registerHandler(channel, handler);
                channel.writeAsyncHashRaw(h, data, size, proxyWriteCompleteHandler);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
        }


        void ChannelWrap::writeAsyncHashHash(karabo::net::Channel& channel, const bp::object& hdr, const bp::object& body, const bp::object& handler) {
            if (bp::extract<Hash>(hdr).check() && bp::extract<Hash>(body).check()) {
                const Hash& header = bp::extract<Hash>(hdr);
                const Hash& data = bp::extract<Hash>(body);
                registerHandler(channel, handler);
                channel.writeAsyncHashHash(header, data, proxyWriteCompleteHandler);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
        }


        void ChannelWrap::setErrorHandler(karabo::net::Channel& channel, const bp::object& handler) {
            registerHandler(channel, handler);
            channel.setErrorHandler(proxyErrorHandler);
        }


        void ChannelWrap::proxyErrorHandler(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& code) {
            CALL_PYTHON_ERROR_HANDLER_WITH_1(bp::object(code));
        }


        void ChannelWrap::waitAsync(karabo::net::Channel& channel, const bp::object& milliobj, const bp::object& handler) {
            if (PyInt_Check(milliobj.ptr())) {
                int milliseconds = bp::extract<int>(milliobj);
                registerHandler(channel, handler);
                channel.waitAsync(milliseconds, proxyWaitCompleteHandler);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
        }


        void ChannelWrap::proxyWaitCompleteHandler(karabo::net::Channel::Pointer channel) {
            CALL_PYTHON_HANDLER_WITH_0();
        }
    }
}