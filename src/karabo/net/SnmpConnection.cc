/* 
 * File:   SnmpConnection.cc
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on July 15, 2011, 2:30 PM
 */

#include <boost/any.hpp>
#include <boost/pointer_cast.hpp>
//#include "IOService.hh"
#include "SnmpIOService.hh"
#include "Channel.hh"
#include "SnmpConnection.hh"
#include "SnmpChannel.hh"

using namespace std;
using namespace exfel::util;

namespace exfel {
    namespace net {

        EXFEL_REGISTER_FACTORY_CC(Connection, SnmpConnection)

        SnmpConnection::SnmpConnection() {
        }

        void SnmpConnection::expectedParameters(exfel::util::Schema& expected) {

            STRING_ELEMENT(expected)
                    .key("hostname")
                    .displayedName("Hostname")
                    .description("IP hostname of a CPU running SNMP Agent")
                    .assignmentOptional().defaultValue("localhost")
                    //.reconfigurable()
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("port")
                    .displayedName("Port")
                    .description("Port listened by SNMP Agent")
                    .assignmentOptional().defaultValue(161)
                    //.reconfigurable()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("version")
                    .displayedName("Version")
                    .description("Version of SNMP protocol")
                    .options("1, 2, 3")
                    .assignmentOptional().defaultValue("2")
                    //.reconfigurable()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("community")
                    .displayedName("Community")
                    .description("Community parameter defining a subset of keys available for retrieving from SNMP Agent")
                    .assignmentOptional().defaultValue("public")
                    //.reconfigurable()
                    .commit();

            /*
             * Behavioral modes for aliases in schema are
             * NoSchema -- no alias conversions, use key "AS IS"
             * Schema   -- use alias conversions for keys having aliases defined and no conversions for others
             * SchemaOnly -- use alias conversions for keys having aliases and silently ignore anything else
             * SchemaOnlyWithException - use alias conversions for keys having aliases and rise an exception for anything else
             */
            STRING_ELEMENT(expected)
                    .key("aliasMode")
                    .displayedName("Alias Mode")
                    .description("Behavior mode of key-to-alias and alias-to-key conversions")
                    .options("NoSchema, Schema, SchemaOnly, SchemaOnlyWithException")
                    .assignmentOptional().defaultValue("NoSchema")
                    //.reconfigurable()
                    .commit();

            INTERNAL_ANY_ELEMENT(expected)
                    .key("schema")
                    .description("Application's schema being passed to SNMP layer")
                    .commit();
        }

        void SnmpConnection::configure(const exfel::util::Hash& input) {
            setIOServiceType("Snmp");
            //m_snmp_service = boost::static_pointer_cast<SnmpService > (m_service->service());

            input.get("hostname", m_hostname);
            input.get("port", m_port);
            std::string version;
            input.get("version", version);
            if (version == "1")
                m_version = SNMP_VERSION_1;
            else if (version == "2")
                m_version = SNMP_VERSION_2c;
            else if (version == "3")
                m_version = SNMP_VERSION_3;
            else
                m_version = SNMP_VERSION_2c;
            input.get("community", m_community);
            input.get("aliasMode", m_aliasMode);
            if (m_aliasMode != "NoSchema") input.get("schema", m_schema);
        }

        Channel::Pointer SnmpConnection::start() {
            Channel::Pointer new_channel = this->createChannel();
            return new_channel;
        }

        void SnmpConnection::startAsync(const ConnectionHandler& handler) {
            Channel::Pointer new_channel = this->createChannel();
            handler(new_channel);
        }

        void SnmpConnection::stop() {
        }

        void SnmpConnection::close() {
        }

        ChannelPointer SnmpConnection::createChannel() {
            ChannelPointer channel(new SnmpChannel(*this, m_hostname, m_port, m_version, m_community));
            registerChannel(channel);
            return channel;
        }
    }
}
