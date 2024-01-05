/*
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
/*
 * File:   ChannelWrap.cc
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 6, 2013, 12:47 AM
 */

#include "ChannelWrap.hh"

#include "ScopedGILAcquire.hh"
#include "Wrapper.hh"

namespace bp = boost::python;
using namespace std;
using namespace karabo::util;
using namespace karabo::net;
using namespace boost::placeholders;

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
            const char* data = PyUnicode_AsUTF8AndSize(obj.ptr(), &size);
            ScopedGILRelease nogil;
            channel->write(data, size);
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Not supported type");
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
                std::string data = bp::extract<std::string>(obj);
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
        throw KARABO_PYTHON_EXCEPTION("Not supported type");
    }


    void ChannelWrap::readAsyncStr(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        using Wrap = HandlerWrapExtra<const ErrorCode&, Channel::Pointer, const std::string&>;
        Wrap proxyWrap(handler, "readAsyncStr", channel);
        ScopedGILRelease nogil;
        channel->readAsyncString(proxyWrap);
    }


    void ChannelWrap::readAsyncHash(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        using Wrap = HandlerWrapExtra<const ErrorCode&, Channel::Pointer, const Hash&>;
        Wrap proxyWrap(handler, "readAsyncHash", channel);
        ScopedGILRelease nogil;
        channel->readAsyncHash(proxyWrap);
    }


    void ChannelWrap::readAsyncHashStr(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        using Wrap = HandlerWrapExtra<const ErrorCode&, Channel::Pointer, const Hash&, const std::string&>;
        Wrap proxyWrap(handler, "readAsyncHashStr", channel);
        ScopedGILRelease nogil;
        channel->readAsyncHashString(proxyWrap);
    }


    void ChannelWrap::readAsyncHashHash(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        using Wrap = HandlerWrapExtra<const ErrorCode&, Channel::Pointer, Hash&, Hash&>;
        Wrap proxyWrap(handler, "readAsyncHashHash", channel);
        ScopedGILRelease nogil;
        channel->readAsyncHashHash(proxyWrap);
    }


    void ChannelWrap::writeAsyncStr(karabo::net::Channel::Pointer channel, const bp::object& obj,
                                    const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");

        using Wrap = HandlerWrapExtra<const ErrorCode&, Channel::Pointer>;
        Wrap proxyWrap(handler, "writeAsyncStr", channel);
        if (PyBytes_Check(obj.ptr())) {
            PyObject* bytearray = obj.ptr();
            size_t size = PyBytes_Size(bytearray);
            char* data = PyBytes_AsString(bytearray);
            ScopedGILRelease nogil;
            channel->writeAsyncRaw(data, size, proxyWrap);
            return;
        }
        if (PyByteArray_Check(obj.ptr())) {
            PyObject* bytearray = obj.ptr();
            size_t size = PyByteArray_Size(bytearray);
            char* data = PyByteArray_AsString(bytearray);
            ScopedGILRelease nogil;
            channel->writeAsyncRaw(data, size, proxyWrap);
            return;
        }
        if (PyUnicode_Check(obj.ptr())) {
            Py_ssize_t size;
            const char* data = PyUnicode_AsUTF8AndSize(obj.ptr(), &size);
            ScopedGILRelease nogil;
            channel->writeAsyncRaw(data, size, proxyWrap);
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Not supported type");
    }


    void ChannelWrap::writeAsyncHash(karabo::net::Channel::Pointer channel, const bp::object& data,
                                     const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        if (bp::extract<Hash>(data).check()) {
            const Hash& h = bp::extract<Hash>(data);
            using Wrap = HandlerWrapExtra<const ErrorCode&, Channel::Pointer>;
            Wrap proxyWrap(handler, "writeAsyncHash", channel);
            ScopedGILRelease nogil;
            channel->writeAsyncHash(h, proxyWrap);
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Not supported type");
    }


    void ChannelWrap::writeAsyncHashStr(karabo::net::Channel::Pointer channel, const bp::object& hdr,
                                        const bp::object& body, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        if (bp::extract<Hash>(hdr).check()) {
            const Hash& h = bp::extract<Hash>(hdr);
            using Wrap = HandlerWrapExtra<const ErrorCode&, Channel::Pointer>;
            Wrap proxyWrap(handler, "writeAsyncHashStr", channel);
            PyObject* bytearray = body.ptr();
            if (PyUnicode_Check(bytearray)) {
                Py_ssize_t size;
                const char* data = PyUnicode_AsUTF8AndSize(bytearray, &size);
                ScopedGILRelease nogil;
                channel->writeAsyncHashRaw(h, data, size, proxyWrap);
                return;
            }
            if (PyBytes_Check(bytearray)) {
                size_t size = PyBytes_Size(bytearray);
                const char* data = PyBytes_AsString(bytearray);
                ScopedGILRelease nogil;
                channel->writeAsyncHashRaw(h, data, size, proxyWrap);
                return;
            }
            if (PyByteArray_Check(bytearray)) {
                size_t size = PyByteArray_Size(bytearray);
                const char* data = PyByteArray_AsString(bytearray);
                ScopedGILRelease nogil;
                channel->writeAsyncHashRaw(h, data, size, proxyWrap);
                return;
            }
        }
        throw KARABO_PYTHON_EXCEPTION("Not supported type");
    }


    void ChannelWrap::writeAsyncHashHash(karabo::net::Channel::Pointer channel, const bp::object& hdr,
                                         const bp::object& body, const bp::object& handler) {
        if (!PyCallable_Check(handler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        if (bp::extract<Hash>(hdr).check() && bp::extract<Hash>(body).check()) {
            const Hash& header = bp::extract<Hash>(hdr);
            const Hash& data = bp::extract<Hash>(body);
            using Wrap = HandlerWrapExtra<const ErrorCode&, Channel::Pointer>;
            Wrap proxyWrap(handler, "writeAsyncHashHash", channel);
            ScopedGILRelease nogil;
            channel->writeAsyncHashHash(header, data, proxyWrap);
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Not supported type");
    }
} // namespace karathon
