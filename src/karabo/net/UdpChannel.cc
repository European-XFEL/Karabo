/* 
 * File:   UdpChannel.cc
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 17, 2011, 3:40 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <karabo/util/Hash.hh>
#include <karabo/io/Reader.hh>
#include <karabo/io/Writer.hh>
#include <boost/asio/placeholders.hpp>
#include "Channel.hh"
#include "UdpChannel.hh"
#include "UdpConnection.hh"

namespace karabo {
    namespace net {

        using namespace std;
        using namespace boost::asio;
        using namespace karabo::util;


        //KARABO_REGISTER_FACTORY_CC(Channel, UdpChannel)

        UdpChannel::UdpChannel(UdpConnection& c) :
        Channel(c),
        m_socket(*(c.m_sock)),
        m_remote_endpoint(c.m_remoteEndpoint),
        m_MaxLength(c.m_MaxLength),
        m_timer(*(c.m_boost_io_service)) {
        }

        UdpChannel::~UdpChannel() {
        }

        void UdpChannel::read(char*& data, size_t& size) {
            try {
                boost::system::error_code ec;
                size = size <= m_MaxLength ? size : m_MaxLength;
                size = m_socket.receive_from(buffer(data, size), m_remote_endpoint, 0, ec);
                if (!ec) return;
                string errmsg(ec.message() + " and transferred " + String::toString(size) + " bytes");
                if (m_errorHandler)
                    m_errorHandler(channel(), errmsg);
                else
                    throw MESSAGE_EXCEPTION(errmsg);
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::read(vector<char>& data) {
            try {
                boost::system::error_code ec;
                if (data.size() < m_MaxLength) data.resize(m_MaxLength);
                size_t size = m_socket.receive_from(buffer(data), m_remote_endpoint, 0, ec);
                if (!ec) return;
                string errmsg(ec.message() + " and transferred " + String::toString(size) + " bytes");
                if (m_errorHandler)
                    m_errorHandler(channel(), errmsg);
                else
                    throw MESSAGE_EXCEPTION(errmsg);
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::readAsyncRaw(char*& data, size_t& size, const ReadRawHandler& handler) {
            try {
                size = size <= m_MaxLength ? size : m_MaxLength;
                m_socket.async_receive_from(buffer(data, size), m_remote_endpoint, boost::bind(
                        &UdpChannel::asyncReadRawHandler, this, data, size, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::asyncReadRawHandler(char*& data, size_t& size, const ReadRawHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                size = bytes_transferred;
                if (!e) {
                    handler(channel(), data, size);
                    return;
                }
                if (m_errorHandler) {
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(size) + " bytes");
                } else {
                    throw MESSAGE_EXCEPTION(e.message());
                }
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::readAsyncVector(const ReadVectorHandler & handler) {
            try {
                if (m_inboundData.size() < m_MaxLength) m_inboundData.resize(m_MaxLength);
                m_socket.async_receive_from(buffer(m_inboundData), m_remote_endpoint,
                        boost::bind(&UdpChannel::asyncReadVectorHandler, this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::asyncReadVectorHandler(const ReadVectorHandler& handler, const boost::system::error_code& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    m_inboundData.resize(bytes_transferred);
                    handler(channel(), m_inboundData);
                } else if (m_errorHandler) {
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes");
                } else {
                    throw MESSAGE_EXCEPTION(e.message());
                }
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::write(const char* data, const size_t & size) {
            try {
                boost::system::error_code ec;
                string errmsg;
                if (size > 0 && size <= m_MaxLength) {
                    m_socket.send_to(buffer(data, size), m_remote_endpoint, 0, ec);
                    if (!ec) return;
                } else {
                    errmsg = "UdpChannel::write:  Data size is out of range";
                }
                if (ec) {
                    errmsg = ec.message();
                }
                if (m_errorHandler) {
                    m_errorHandler(channel(), errmsg);
                } else {
                    throw MESSAGE_EXCEPTION(errmsg);
                }
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::write(const vector<char>& data) {
            try {
                write(&data[0], data.size());
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::writeAsyncVector(const vector<char>& data, const WriteCompleteHandler & handler) {
            try {
                const size_t size = data.size();
                writeAsyncRaw(&data[0], size, handler);
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler & handler) {
            try {
                if (size > 0 && size <= m_MaxLength) {
                    m_socket.async_send_to(buffer(data, size), m_remote_endpoint,
                            boost::bind(&UdpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
                } else {
                    string errmsg("UdpChannel::writeAsyncRaw: Data size is out of range");
                    if (m_errorHandler) {
                        m_errorHandler(channel(), errmsg);
                    } else {
                        throw MESSAGE_EXCEPTION(errmsg);
                    }
                }
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::asyncWriteHandler(const WriteCompleteHandler& handler, const ErrorCode & e) {
            try {
                if (!e) {
                    handler(channel());
                } else {
                    if (m_errorHandler)
                        m_errorHandler(channel(), e.message());
                    else
                        throw MESSAGE_EXCEPTION(e.message());
                }
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::waitAsync(int millisecs, const WaitHandler & handler) {
            m_timer.expires_from_now(boost::posix_time::milliseconds(millisecs));
            m_timer.async_wait(boost::bind(&UdpChannel::asyncWaitHandler, this, handler, boost::asio::placeholders::error));
        }

        void UdpChannel::asyncWaitHandler(const Channel::WaitHandler& handler, const ErrorCode & e) {
            try {
                if (!e)
                    handler(channel());
                else if (m_errorHandler)
                    m_errorHandler(channel(), e.message());
                else
                    throw boost::system::system_error(e);
            } catch (...) {
                RETHROW
            }
        }

        void UdpChannel::close() {
            m_socket.close();
        }

        bool UdpChannel::isOpen() {
            return m_socket.is_open();
        }
    }
}

