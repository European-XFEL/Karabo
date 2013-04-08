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
                ScopedGILRelease nogil;
                return channel.readSizeInBytes();
            }

            static void read(karabo::net::Channel& channel, bp::object& obj);
            static void read2(karabo::net::Channel& channel, bp::object& header, bp::object& obj);
            static void write(karabo::net::Channel& channel, const bp::object& obj);
            static void write2(karabo::net::Channel& channel, const bp::object& header, const bp::object& obj);
            static void readAsyncSizeInBytes(karabo::net::Channel& channel, const bp::object& handler);
            static void readAsyncStr(karabo::net::Channel& channel, bp::object& obj, const bp::object& handler);
            static void readAsyncHash(karabo::net::Channel& channel, const bp::object& handler);
            static void readAsyncHashStr(karabo::net::Channel& channel, const bp::object& handler);
            static void readAsyncHashHash(karabo::net::Channel& channel, const bp::object& handler);
            static void writeAsyncStr(karabo::net::Channel& channel, const bp::object& data, const bp::object& handler);
            static void writeAsyncHash(karabo::net::Channel& channel, const bp::object& data, const bp::object& handler);
            static void writeAsyncHashStr(karabo::net::Channel& channel, const bp::object& hdr, const bp::object& data, const bp::object& handler);
            static void writeAsyncHashHash(karabo::net::Channel& channel, const bp::object& hdr, const bp::object& data, const bp::object& handler);
            static void waitAsync(karabo::net::Channel& channel, const bp::object& milliseconds, const bp::object& handler);
            static void setErrorHandler(karabo::net::Channel& channel, const bp::object& handler);
            
        private:
            static void registerHandler(karabo::net::Channel& channel, const bp::object& handler);

            static void proxyReadSizeInBytesHandler(karabo::net::Channel::Pointer channel, const size_t& size);
            static void proxyReadRawHandler(karabo::net::Channel::Pointer channel);
            static void proxyReadHashHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash& hash);
            static void proxyReadHashVectorHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash& hash, const std::vector<char>& v);
            static void proxyReadHashHashHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash& h, const karabo::util::Hash& b);
            static void proxyWriteCompleteHandler(karabo::net::Channel::Pointer channel);
            static void proxyWaitCompleteHandler(karabo::net::Channel::Pointer channel);
            static void proxyErrorHandler(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& code);

            // I've taken this helper function from Burkhard

            static bool hasattr(bp::object obj, const std::string& attrName) {
                return PyObject_HasAttrString(obj.ptr(), const_cast<char*> (attrName.c_str()));
            }

        private:
            static boost::mutex m_changedChannelHandlersMutex;
            static std::map<karabo::net::Channel*, karabo::util::Hash> m_channelHandlers;
        };

    }
}
#endif	/* KARABO_PYEXFEL_CHANNELWRAP_HH */

