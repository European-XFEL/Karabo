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

namespace karathon {


    bp::object ChannelWrap::readStr(karabo::net::Channel::Pointer channel) {
        char* data = 0;
        size_t size = 0;
        {
            ScopedGILRelease nogil;
            size = channel->readSizeInBytes();
            data = new char[size];
            channel->read(data, size);
        }
        PyObject* pyobj = PyUnicode_FromStringAndSize(data, size);
        return bp::object(bp::handle<>(pyobj));
    }


    bp::object ChannelWrap::readHash(karabo::net::Channel::Pointer channel) {
        boost::shared_ptr<Hash> hash(new Hash());
        {
            ScopedGILRelease nogil;
            channel->read(*hash);
        }
        return bp::object(hash);
    }


    bp::tuple ChannelWrap::readHashStr(karabo::net::Channel::Pointer channel) {
        boost::shared_ptr<Hash> header(new Hash());
        char* data = 0;
        size_t size = 0;
        {
            ScopedGILRelease nogil;
            channel->read(*header);
            size = channel->readSizeInBytes();
            data = new char[size];
            channel->read(data, size);
        }
        PyObject* pyobj = PyUnicode_FromStringAndSize(data, size);
        return bp::make_tuple(bp::object(header), bp::object(bp::handle<>(pyobj)));
    }


    bp::tuple ChannelWrap::readHashHash(karabo::net::Channel::Pointer channel) {
        boost::shared_ptr<Hash> header(new Hash());
        boost::shared_ptr<Hash> body(new Hash());
        {
            ScopedGILRelease nogil;
            channel->read(*header);
            channel->read(*body);
        }
        return bp::make_tuple(bp::object(header), bp::object(body));
    }


    void ChannelWrap::write(karabo::net::Channel::Pointer channel, const bp::object& obj) {
        if (PyBytes_Check(obj.ptr())) {
            PyObject* bytearray = obj.ptr();
            size_t size = PyBytes_Size(bytearray);
            char* data = PyBytes_AsString(bytearray);
            ScopedGILRelease nogil;
            channel->write(data, size);
            return;
        } else if (PyByteArray_Check(obj.ptr())) {
            PyObject* bytearray = obj.ptr();
            size_t size = PyByteArray_Size(bytearray);
            char* data = PyByteArray_AsString(bytearray);
            ScopedGILRelease nogil;
            channel->write(data, size);
            return;
        } else if (bp::extract<Hash&>(obj).check()) {
            const Hash& hash = bp::extract<const Hash&>(obj);
            ScopedGILRelease nogil;
            channel->write(hash);
            return;
        } else if (PyUnicode_Check(obj.ptr())) {
            Py_ssize_t size;
            char* data = PyUnicode_AsUTF8AndSize(obj.ptr(), &size);
            ScopedGILRelease nogil;
            channel->write(data, size);
            return;
        }
        PyErr_SetString(PyExc_TypeError, "Python type in parameters is not supported");
    }


    void ChannelWrap::write2(karabo::net::Channel::Pointer channel, const bp::object& header, const bp::object& obj) {
        if (bp::extract<karabo::util::Hash>(header).check()) {
            const karabo::util::Hash hdr = bp::extract<const karabo::util::Hash&>(header);
            if (PyBytes_Check(obj.ptr())) {
                PyObject* bytearray = obj.ptr();
                size_t size = PyBytes_Size(bytearray);
                char* data = PyBytes_AsString(bytearray);
                ScopedGILRelease nogil;
                channel->write(hdr, data, size);
                return;
            } else if (PyByteArray_Check(obj.ptr())) {
                PyObject* bytearray = obj.ptr();
                size_t size = PyByteArray_Size(bytearray);
                char* data = PyByteArray_AsString(bytearray);
                ScopedGILRelease nogil;
                channel->write(hdr, data, size);
                return;
            } else if (PyUnicode_Check(obj.ptr())) {
                std::string data = bp::extract<std::string >(obj);
                ScopedGILRelease nogil;
                channel->write(hdr, data);
                return;
            } else if (bp::extract<karabo::util::Hash&>(obj).check()) {
                const karabo::util::Hash& hash = bp::extract<const karabo::util::Hash&>(obj);
                ScopedGILRelease nogil;
                channel->write(hdr, hash);
                return;
            }

        }
        PyErr_SetString(PyExc_TypeError, "Python types in parameters are not supported");
    }


    void ChannelWrap::readAsyncSizeInBytes(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        ScopedGILRelease nogil;
        channel->readAsyncSizeInBytes(boost::bind(proxyReadSizeInBytesHandler, handler, channel, _1));
    }


    void ChannelWrap::proxyReadSizeInBytesHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const size_t& size) {
        ScopedGILAcquire gil;
        try {
            handler(bp::object(channel), bp::object(size));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("ReadSizeInBytesHandler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }


    void ChannelWrap::readAsyncStr(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        ScopedGILRelease nogil;
        channel->readAsyncString(boost::bind(proxyReadStringHandler, handler, channel, _1));
    }


    void ChannelWrap::proxyReadStringHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const std::string& s) {
        ScopedGILAcquire gil;
        try {
            handler(bp::object(channel), bp::object(s));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("ReadStringHandler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }


    void ChannelWrap::readAsyncHash(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        ScopedGILRelease nogil;
        channel->readAsyncHash(boost::bind(proxyReadHashHandler, handler, channel, _1));
    }


    void ChannelWrap::proxyReadHashHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const karabo::util::Hash& h) {
        ScopedGILAcquire gil;
        try {
            handler(bp::object(channel), bp::object(h));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("ReadHashHandler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }


    void ChannelWrap::readAsyncHashStr(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        ScopedGILRelease nogil;
        channel->readAsyncHashVector(boost::bind(proxyReadHashVectorHandler, handler, channel, _1, _2));
    }


    void ChannelWrap::proxyReadHashVectorHandler(const bp::object& handler, karabo::net::Channel::Pointer channel,
                                                 const karabo::util::Hash& h, const std::vector<char>& v) {
        ScopedGILAcquire gil;
        try {
            handler(bp::object(channel), bp::object(h), bp::object(v));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("ReadHashVectorHandler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }


    void ChannelWrap::readAsyncHashHash(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        ScopedGILRelease nogil;
        channel->readAsyncHashHash(boost::bind(proxyReadHashHashHandler, handler, channel, _1, _2));
    }


    void ChannelWrap::proxyReadHashHashHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const karabo::util::Hash& h, const karabo::util::Hash& b) {
        ScopedGILAcquire gil;
        try {
            handler(bp::object(channel), bp::object(h), bp::object(b));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("ReadHashHashHandler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }


    void ChannelWrap::writeAsyncStr(karabo::net::Channel::Pointer channel, const bp::object& obj, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        if (PyBytes_Check(obj.ptr())) {
            PyObject* bytearray = obj.ptr();
            size_t size = PyBytes_Size(bytearray);
            char* data = PyBytes_AsString(bytearray);
            ScopedGILRelease nogil;
            channel->writeAsyncRaw(data, size, boost::bind(proxyWriteCompleteHandler, handler, channel));
            return;
        } else if (PyByteArray_Check(obj.ptr())) {
            PyObject* bytearray = obj.ptr();
            size_t size = PyByteArray_Size(bytearray);
            char* data = PyByteArray_AsString(bytearray);
            ScopedGILRelease nogil;
            channel->writeAsyncRaw(data, size, boost::bind(proxyWriteCompleteHandler, handler, channel));
            return;
        } else if (PyUnicode_Check(obj.ptr())) {
            Py_ssize_t size;
            const char* data = PyUnicode_AsUTF8AndSize(obj.ptr(), &size);
            ScopedGILRelease nogil;
            channel->writeAsyncRaw(data, size, boost::bind(proxyWriteCompleteHandler, handler, channel));
            return;
        }
        PyErr_SetString(PyExc_TypeError, "Python type in parameters is not supported");
    }


    void ChannelWrap::proxyWriteCompleteHandler(const bp::object& handler, karabo::net::Channel::Pointer channel) {
        ScopedGILAcquire gil;
        try {
            handler(bp::object(channel));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("WriteCompleteHandler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }


    void ChannelWrap::writeAsyncHash(karabo::net::Channel::Pointer channel, const bp::object& data, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        if (bp::extract<Hash>(data).check()) {
            const Hash& h = bp::extract<Hash>(data);
            ScopedGILRelease nogil;
            channel->writeAsyncHash(h, boost::bind(proxyWriteCompleteHandler, handler, channel));
            return;
        }
        PyErr_SetString(PyExc_TypeError, "Python type in parameters is not supported");
    }


    void ChannelWrap::writeAsyncHashStr(karabo::net::Channel::Pointer channel, const bp::object& hdr, const bp::object& body, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        if (bp::extract<Hash>(hdr).check()) {
            const Hash& h = bp::extract<Hash>(hdr);
            PyObject* bytearray = body.ptr();
            size_t size = 0;
            char* data = 0;
            if (PyBytes_Check(bytearray)) {
                size = PyBytes_Size(bytearray);
                data = PyBytes_AsString(bytearray);
            } else if (PyByteArray_Check(bytearray)) {
                size = PyByteArray_Size(bytearray);
                data = PyByteArray_AsString(bytearray);
            } else {
                PyErr_SetString(PyExc_TypeError, "Python type in parameters is not supported");
            }
            ScopedGILRelease nogil;
            channel->writeAsyncHashRaw(h, data, size, boost::bind(proxyWriteCompleteHandler, handler, channel));
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
    }


    void ChannelWrap::writeAsyncHashHash(karabo::net::Channel::Pointer channel, const bp::object& hdr, const bp::object& body, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        if (bp::extract<Hash>(hdr).check() && bp::extract<Hash>(body).check()) {
            const Hash& header = bp::extract<Hash>(hdr);
            const Hash& data = bp::extract<Hash>(body);
            ScopedGILRelease nogil;
            channel->writeAsyncHashHash(header, data, boost::bind(proxyWriteCompleteHandler, handler, channel));
            return;
        }
        PyErr_SetString(PyExc_TypeError, "Python type in parameters is not supported");
    }


    void ChannelWrap::setErrorHandler(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        ScopedGILRelease nogil;
        channel->setErrorHandler(boost::bind(proxyErrorHandler, handler, channel, _1));
    }


    void ChannelWrap::proxyErrorHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& code) {
        ScopedGILAcquire gil;
        try {
            handler(bp::object(channel), bp::object(code));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("ErrorHandler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }


    void ChannelWrap::waitAsync(karabo::net::Channel::Pointer channel, const bp::object& milliobj, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        if (PyLong_Check(milliobj.ptr())) {
            int milliseconds = bp::extract<int>(milliobj);
            ScopedGILRelease nogil;
            channel->waitAsync(milliseconds, boost::bind(proxyWaitCompleteHandler, handler, channel));
            return;
        }
        PyErr_SetString(PyExc_TypeError, "Python type in parameters is not supported");
    }


    void ChannelWrap::proxyWaitCompleteHandler(const bp::object& handler, karabo::net::Channel::Pointer channel) {
        ScopedGILAcquire gil;
        try {
            handler(bp::object(channel));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("WaitCompleteHandler has thrown an exception.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }
}
