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

std::map<karabo::net::IOService*, std::map<karabo::net::Channel*, karabo::util::Hash> > karathon::ChannelWrap::m_handlers;
boost::mutex karathon::ChannelWrap::m_changedHandlersMutex;

namespace karathon {

    void ChannelWrap::registerReadHandler(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        Connection::Pointer connection = channel->getConnection();
        IOService::Pointer ioserv = connection->getIOService();
        boost::mutex::scoped_lock lock(m_changedHandlersMutex);
        map<IOService*, map<Channel*, Hash> >::iterator it = m_handlers.find(ioserv.get());
        if (it == m_handlers.end()) m_handlers[ioserv.get()] = map<Channel*, Hash>();
        map<Channel*, Hash>& cmap = m_handlers[ioserv.get()];
        map<Channel*, Hash>::iterator ii = cmap.find(channel.get());
        if (ii == cmap.end()) cmap[channel.get()] = Hash();
        Hash& hash = cmap[channel.get()];
        hash.set("_read", handler); // register python read handler for channel
    }

    void ChannelWrap::registerWriteHandler(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        Connection::Pointer connection = channel->getConnection();
        IOService::Pointer ioserv = connection->getIOService();
        boost::mutex::scoped_lock lock(m_changedHandlersMutex);
        map<IOService*, map<Channel*, Hash> >::iterator it = m_handlers.find(ioserv.get());
        if (it == m_handlers.end()) m_handlers[ioserv.get()] = map<Channel*, Hash>();
        map<Channel*, Hash>& cmap = m_handlers[ioserv.get()];
        map<Channel*, Hash>::iterator ii = cmap.find(channel.get());
        if (ii == cmap.end()) cmap[channel.get()] = Hash();
        Hash& hash = cmap[channel.get()];
        hash.set("_write", handler); // register python write handler for channel
    }

    void ChannelWrap::registerErrorHandler(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        Connection::Pointer connection = channel->getConnection();
        IOService::Pointer ioserv = connection->getIOService();
        boost::mutex::scoped_lock lock(m_changedHandlersMutex);
        map<IOService*, map<Channel*, Hash> >::iterator it = m_handlers.find(ioserv.get());
        if (it == m_handlers.end()) m_handlers[ioserv.get()] = map<Channel*, Hash>();
        map<Channel*, Hash>& cmap = m_handlers[ioserv.get()];
        map<Channel*, Hash>::iterator ii = cmap.find(channel.get());
        if (ii == cmap.end()) cmap[channel.get()] = Hash();
        Hash& hash = cmap[channel.get()];
        hash.set("_error", handler); // register python write handler for channel
    }

    void ChannelWrap::registerWaitHandler(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        Connection::Pointer connection = channel->getConnection();
        IOService::Pointer ioserv = connection->getIOService();
        boost::mutex::scoped_lock lock(m_changedHandlersMutex);
        map<IOService*, map<Channel*, Hash> >::iterator it = m_handlers.find(ioserv.get());
        if (it == m_handlers.end()) m_handlers[ioserv.get()] = map<Channel*, Hash>();
        map<Channel*, Hash>& cmap = m_handlers[ioserv.get()];
        map<Channel*, Hash>::iterator ii = cmap.find(channel.get());
        if (ii == cmap.end()) cmap[channel.get()] = Hash();
        Hash& hash = cmap[channel.get()];
        hash.set("_wait", handler); // register python write handler for channel
    }

    bp::object ChannelWrap::getPythonReadHandler(karabo::net::Channel::Pointer channel) {
        Connection::Pointer connection = channel->getConnection();
        IOService::Pointer ioserv = connection->getIOService();
        boost::mutex::scoped_lock lock(m_changedHandlersMutex);
        map<IOService*, map<Channel*, Hash> >::iterator it = m_handlers.find(ioserv.get());
        if (it == m_handlers.end()) return bp::object();
        map<Channel*, Hash>& cmap = it->second; // reference to internal connections map
        map<Channel*, Hash>::iterator ii = cmap.find(channel.get());
        if (ii == cmap.end()) return bp::object();
        Hash& h = ii->second;
        if (!h.has("_read"))
            throw KARABO_PYTHON_EXCEPTION("Logical error: Read handler's registration is not found");
        bp::object onread = h.get<bp::object>("_read");
        h.erase("_read");
        return onread;
    }

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
        throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
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
        throw KARABO_PYTHON_EXCEPTION("Python types in parameters are not supported");

    }

    void ChannelWrap::readAsyncSizeInBytes(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        registerReadHandler(channel, handler);
        ScopedGILRelease nogil;
        channel->readAsyncSizeInBytes(boost::bind(proxyReadSizeInBytesHandler, channel, _1));
    }

    void ChannelWrap::proxyReadSizeInBytesHandler(karabo::net::Channel::Pointer channel, const size_t& size) {
        try {
            ScopedGILAcquire gil;
            bp::object handler = getPythonReadHandler(channel);
            if (handler == bp::object())
                throw KARABO_PYTHON_EXCEPTION("proxyReadSizeInBytesHandler: Python handler was not found.");
            try {
                handler(bp::object(channel), bp::object(size));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
            }
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void ChannelWrap::readAsyncStr(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        registerReadHandler(channel, handler);
        ScopedGILRelease nogil;
        channel->readAsyncString(boost::bind(proxyReadStringHandler, channel, _1));
    }

    void ChannelWrap::proxyReadStringHandler(karabo::net::Channel::Pointer channel, const std::string& s) {
        ScopedGILAcquire gil;
        try {
            bp::object handler = getPythonReadHandler(channel);
            if (handler == bp::object())
                throw KARABO_PYTHON_EXCEPTION("proxyReadStringHandler: Python handler was not found.");
            try {
                handler(bp::object(channel), bp::object(s));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
            }
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void ChannelWrap::readAsyncHash(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        registerReadHandler(channel, handler);
        ScopedGILRelease nogil;
        channel->readAsyncHash(boost::bind(proxyReadHashHandler, channel, _1));
    }

    void ChannelWrap::proxyReadHashHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash& h) {
        try {
            ScopedGILAcquire gil;
            bp::object handler = getPythonReadHandler(channel);
            if (handler == bp::object())
                throw KARABO_PYTHON_EXCEPTION("proxyReadHashHandler: Python handler was not found.");
            try {
                handler(bp::object(channel), bp::object(h));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
            }
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void ChannelWrap::readAsyncHashStr(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        registerReadHandler(channel, handler);
        ScopedGILRelease nogil;
        channel->readAsyncHashVector(boost::bind(proxyReadHashVectorHandler, channel, _1, _2));
    }

    void ChannelWrap::proxyReadHashVectorHandler(karabo::net::Channel::Pointer channel,
            const karabo::util::Hash& h, const std::vector<char>& v) {
        try {
            ScopedGILAcquire gil;
            bp::object handler = getPythonReadHandler(channel);
            if (handler == bp::object())
                throw KARABO_PYTHON_EXCEPTION("proxyReadHashVectorHandler: Python handler was not found.");
            try {
                handler(bp::object(channel), bp::object(h), bp::object(v));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
            }
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void ChannelWrap::readAsyncHashHash(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        registerReadHandler(channel, handler);
        ScopedGILRelease nogil;
        channel->readAsyncHashHash(boost::bind(proxyReadHashHashHandler, channel, _1, _2));
    }

    void ChannelWrap::proxyReadHashHashHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash& h, const karabo::util::Hash& b) {
        try {
            ScopedGILAcquire gil;
            bp::object handler = getPythonReadHandler(channel);
            if (handler == bp::object())
                throw KARABO_PYTHON_EXCEPTION("proxyReadHashHashHandler: Python handler was not found.");
            try {
                handler(bp::object(channel), bp::object(h), bp::object(b));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
            }
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void ChannelWrap::writeAsyncStr(karabo::net::Channel::Pointer channel, const bp::object& obj, const bp::object& handler) {
        if (PyBytes_Check(obj.ptr())) {
            PyObject* bytearray = obj.ptr();
            size_t size = PyBytes_Size(bytearray);
            char* data = PyBytes_AsString(bytearray);
            registerWriteHandler(channel, handler);
            ScopedGILRelease nogil;
            channel->writeAsyncRaw(data, size, boost::bind(proxyWriteCompleteHandler, channel));
            return;
        } else if (PyByteArray_Check(obj.ptr())) {
            PyObject* bytearray = obj.ptr();
            size_t size = PyByteArray_Size(bytearray);
            char* data = PyByteArray_AsString(bytearray);
            registerWriteHandler(channel, handler);
            ScopedGILRelease nogil;
            channel->writeAsyncRaw(data, size, boost::bind(proxyWriteCompleteHandler, channel));
            return;
        } else if (PyUnicode_Check(obj.ptr())) {
            Py_ssize_t size;
            const char* data = PyUnicode_AsUTF8AndSize(obj.ptr(), &size);
            registerWriteHandler(channel, handler);
            ScopedGILRelease nogil;
            channel->writeAsyncRaw(data, size, boost::bind(proxyWriteCompleteHandler, channel));
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
    }

    void ChannelWrap::proxyWriteCompleteHandler(karabo::net::Channel::Pointer channel) {
        Connection::Pointer connection = channel->getConnection();
        IOService::Pointer ioserv = connection->getIOService();
        ScopedGILAcquire gil;
        bp::object onwrite;
        try {
            boost::mutex::scoped_lock lock(m_changedHandlersMutex);
            map<IOService*, map<Channel*, Hash> >::iterator it = m_handlers.find(ioserv.get());
            if (it == m_handlers.end()) return;
            map<Channel*, Hash>& cmap = it->second; // reference to internal connections map
            map<Channel*, Hash>::iterator ii = cmap.find(channel.get());
            if (ii == cmap.end()) return;
            Hash& h = ii->second;
            if (!h.has("_write"))
                throw KARABO_PYTHON_EXCEPTION("Logical error: WriteComplete handler's registration is not found");
            onwrite = h.get<bp::object>("_write");
            h.erase("_write");
        } catch (...) {
            KARABO_RETHROW
        }
        try {
            if (onwrite == bp::object()) {
                throw KARABO_PYTHON_EXCEPTION("Cannot execute WriteComplete handler: registration was invalid.");
            }
            onwrite(bp::object(channel));
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }

    void ChannelWrap::writeAsyncHash(karabo::net::Channel::Pointer channel, const bp::object& data, const bp::object& handler) {
        if (bp::extract<Hash>(data).check()) {
            const Hash& h = bp::extract<Hash>(data);
            registerWriteHandler(channel, handler);
            ScopedGILRelease nogil;
            channel->writeAsyncHash(h, boost::bind(proxyWriteCompleteHandler, channel));
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
    }

    void ChannelWrap::writeAsyncHashStr(karabo::net::Channel::Pointer channel, const bp::object& hdr, const bp::object& body, const bp::object& handler) {
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
                throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
            }
            registerWriteHandler(channel, handler);
            ScopedGILRelease nogil;
            channel->writeAsyncHashRaw(h, data, size, boost::bind(proxyWriteCompleteHandler, channel));
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
    }

    void ChannelWrap::writeAsyncHashHash(karabo::net::Channel::Pointer channel, const bp::object& hdr, const bp::object& body, const bp::object& handler) {
        if (bp::extract<Hash>(hdr).check() && bp::extract<Hash>(body).check()) {
            const Hash& header = bp::extract<Hash>(hdr);
            const Hash& data = bp::extract<Hash>(body);
            registerWriteHandler(channel, handler);
            ScopedGILRelease nogil;
            channel->writeAsyncHashHash(header, data, boost::bind(proxyWriteCompleteHandler, channel));
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
    }

    void ChannelWrap::setErrorHandler(karabo::net::Channel::Pointer channel, const bp::object& handler) {
        registerErrorHandler(channel, handler);
        ScopedGILRelease nogil;
        channel->setErrorHandler(boost::bind(proxyErrorHandler, channel, _1));
    }

    void ChannelWrap::proxyErrorHandler(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& code) {
        Connection::Pointer connection = channel->getConnection();
        IOService::Pointer ioserv = connection->getIOService();
        try {
            ScopedGILAcquire gil;
            bp::object onerr;
            {
                boost::mutex::scoped_lock lock(m_changedHandlersMutex);
                map<IOService*, map<Channel*, Hash> >::iterator it = m_handlers.find(ioserv.get());
                if (it == m_handlers.end()) return;
                map<Channel*, Hash>& cmap = it->second; // reference to internal connections map
                map<Channel*, Hash>::iterator ii = cmap.find(channel.get());
                if (ii == cmap.end()) return;
                Hash& h = ii->second;
                if (!h.has("_error"))
                    throw KARABO_PYTHON_EXCEPTION("Logical error: Error handler's registration is not found");
                onerr = h.get<bp::object>("_error");
                //h.erase("_error");     <--- comment it:  error handler lives forever
            }
            try {
                onerr(bp::object(channel), bp::object(code));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
            }
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void ChannelWrap::waitAsync(karabo::net::Channel::Pointer channel, const bp::object& milliobj, const bp::object& handler, const std::string& id) {
        if (PyLong_Check(milliobj.ptr())) {
            int milliseconds = bp::extract<int>(milliobj);
            registerWaitHandler(channel, handler);
            ScopedGILRelease nogil;
            channel->waitAsync(milliseconds, boost::bind(proxyWaitCompleteHandler, channel, _1), id);
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type in parameter is not supported");
    }

    void ChannelWrap::proxyWaitCompleteHandler(karabo::net::Channel::Pointer channel, const std::string& id) {
        Connection::Pointer connection = channel->getConnection();
        IOService::Pointer ioserv = connection->getIOService();
        try {
            ScopedGILAcquire gil;
            bp::object onwait;
            try {
                boost::mutex::scoped_lock lock(m_changedHandlersMutex);
                map<IOService*, map<Channel*, Hash> >::iterator it = m_handlers.find(ioserv.get());
                if (it == m_handlers.end()) return;
                map<Channel*, Hash>& cmap = it->second; // reference to internal connections map
                map<Channel*, Hash>::iterator ii = cmap.find(channel.get());
                if (ii == cmap.end()) return;
                Hash& h = ii->second;
                if (!h.has("_wait"))
                    throw KARABO_PYTHON_EXCEPTION("Logical error: WaitComplete handler's registration is not found");
                onwait = h.get<bp::object>("_wait");
                //h.erase("_error");     <--- comment it:  error handler lives forever
            } catch (...) {
                KARABO_RETHROW
            }
            try {
                onwait(bp::object(channel), id);
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
            }
        } catch (...) {
            KARABO_RETHROW
        }
    }
}
