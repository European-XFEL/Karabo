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

namespace karathon {

    class ChannelWrap {

    public:

        static bp::object getConnection(karabo::net::Channel::Pointer channel) {
            karabo::net::Connection::Pointer connection;
            {
                ScopedGILRelease nogil;
                connection = channel->getConnection();
            }
            return bp::object(connection);
        }

        static size_t readSizeInBytes(karabo::net::Channel::Pointer channel) {
            ScopedGILRelease nogil;
            return channel->readSizeInBytes();
        }

        static bp::object readStr(karabo::net::Channel::Pointer channel);
        static bp::object readHash(karabo::net::Channel::Pointer channel);
        static bp::tuple readHashStr(karabo::net::Channel::Pointer channel);
        static bp::tuple readHashHash(karabo::net::Channel::Pointer channel);
        static void write(karabo::net::Channel::Pointer channel, const bp::object& obj);
        static void write2(karabo::net::Channel::Pointer channel, const bp::object& header, const bp::object& obj);
        static void readAsyncSizeInBytes(karabo::net::Channel::Pointer channel, const bp::object& handler);
        static void readAsyncStr(karabo::net::Channel::Pointer channel, const bp::object& handler);
        static void readAsyncHash(karabo::net::Channel::Pointer channel, const bp::object& handler);
        static void readAsyncHashStr(karabo::net::Channel::Pointer channel, const bp::object& handler);
        static void readAsyncHashHash(karabo::net::Channel::Pointer channel, const bp::object& handler);
        static void writeAsyncStr(karabo::net::Channel::Pointer channel, const bp::object& data, const bp::object& handler);
        static void writeAsyncHash(karabo::net::Channel::Pointer channel, const bp::object& data, const bp::object& handler);
        static void writeAsyncHashStr(karabo::net::Channel::Pointer channel, const bp::object& hdr, const bp::object& data, const bp::object& handler);
        static void writeAsyncHashHash(karabo::net::Channel::Pointer channel, const bp::object& hdr, const bp::object& data, const bp::object& handler);
        static void waitAsync(karabo::net::Channel::Pointer channel, const bp::object& milliseconds, const bp::object& handler, const std::string& id);
        static void setErrorHandler(karabo::net::Channel::Pointer channel, const bp::object& handler);

        static size_t id(karabo::net::Channel::Pointer channel) {
            return size_t(&(*channel));
        }

        static void clear(karabo::net::IOService::Pointer ioserv) {
        }

    private:

        static void proxyReadSizeInBytesHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const size_t& size);
        static void proxyReadStringHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const std::string& s);
        static void proxyReadHashHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const karabo::util::Hash& hash);
        static void proxyReadHashVectorHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const karabo::util::Hash& hash, const std::vector<char>& v);
        static void proxyReadHashHashHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const karabo::util::Hash& h, const karabo::util::Hash& b);
        static void proxyWriteCompleteHandler(const bp::object& handler, karabo::net::Channel::Pointer channel);
        static void proxyWaitCompleteHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const std::string& id);
        static void proxyErrorHandler(const bp::object& handler, karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& code);
        static bool hasattr(bp::object obj, const std::string& attrName) {
            return PyObject_HasAttrString(obj.ptr(), const_cast<char*> (attrName.c_str()));
        }
    };

}
#endif	/* KARABO_PYEXFEL_CHANNELWRAP_HH */

