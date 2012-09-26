/* 
 * File:   SnmpChannel.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on July 14, 2011, 5:54 PM
 */

#ifndef KARABO_NET_SNMPCHANNEL_HH
#define	KARABO_NET_SNMPCHANNEL_HH

#include "Channel.hh"

// WARNING: THIS INCLUDE STATEMENT HAS TO BE LAST AS IT DEFINES READ AS A MACRO
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>


//#include "SnmpIOService.hh"

namespace karabo {
    namespace net {

        class SnmpConnection;
        
        class SnmpIOService;
        
        enum AliasFlag { NoSchema = 0, Schema, SchemaOnly, SchemaOnlyWithException };
        
        class SnmpChannel : public Channel, public boost::enable_shared_from_this<SnmpChannel> {
            
            typedef boost::shared_ptr<SnmpIOService> SnmpIOServicePointer;

        public:

            KARABO_CLASSINFO(SnmpChannel, "SnmpChannel", "1.0")

            typedef boost::shared_ptr<SnmpChannel> Pointer;

            // external functions with an access to private members
            friend int async_snmpget_callback(int, snmp_session*, int, snmp_pdu*, void*);
            friend int async_snmpwalk_callback(int, snmp_session*, int, snmp_pdu*, void*);
            friend int async_snmpgetbulk_callback(int, snmp_session*, int, snmp_pdu*, void*);
            friend int async_snmpwalkbulk_callback(int, snmp_session*, int, snmp_pdu*, void*);

            SnmpChannel(SnmpConnection& c, const std::string& hostname,
                    unsigned int port, long version, const std::string& community);
            virtual ~SnmpChannel();

            SnmpChannel::Pointer channel() {
                return shared_from_this();
            }

            virtual void read(std::vector<char>& cmd, karabo::util::Hash& hash);
            virtual void read(std::string& cmd, karabo::util::Hash& hash);
            virtual void read(karabo::util::Hash& hash);
            
            virtual void readAsyncVectorHash(const ReadVectorHashHandler& handler);
            
            virtual void write(const std::vector<char>& cmd, const karabo::util::Hash& hash);
            virtual void write(const std::string& cmd, const karabo::util::Hash& hash);
            virtual void write(const karabo::util::Hash& hash);
            
            virtual void writeAsyncVectorHash(const std::vector<char>& cmd, const karabo::util::Hash& hash, const WriteCompleteHandler& handler);
            virtual void writeAsyncStringHash(const std::string& cmd, const karabo::util::Hash& hash, const WriteCompleteHandler& handler);
            virtual void writeAsyncHash(const karabo::util::Hash& hash, const WriteCompleteHandler& handler);

            virtual void setErrorHandler(const ErrorHandler& handler); 
                
            virtual void close();

        private:

            static int async_snmpget_callback(int op, snmp_session *sp, int reqid, snmp_pdu *response, void *cbdata);
            static int async_snmpwalk_callback(int op, snmp_session *sp, int reqid, snmp_pdu *response, void *cbdata);
            static int async_snmpgetbulk_callback(int op, snmp_session *sp, int reqid, snmp_pdu *response, void *cbdata);
            static int async_snmpwalkbulk_callback(int op, snmp_session *sp, int reqid, snmp_pdu *response, void *cbdata);

            void requestMoreDataAsync();
            void convertVarbindToHash(netsnmp_variable_list* v, karabo::util::Hash& hash);
            void read_snmpget(karabo::util::Hash& hash);
            void read_snmpwalk(karabo::util::Hash& hash);
            void read_snmpwalkbulk(karabo::util::Hash& hash);

        private:
            /*
             * Converts a device parameter key into its aliased key (must be defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return Aliased representation of the parameter
             */
            template <class T>
            const T& key2alias(const std::string& key);

            /*
             * Converts a device parameter alias into the original key (must be defined in the expectedParameters function)
             * @param key A valid parameter-alias of the device (must be defined in the expectedParameters function)
             * @return The original name of the parameter
             */
            template <class T> const std::string& alias2key(const T& alias);
            bool hasKey(const std::string& key);
            bool keyHasAlias(const std::string& key);
            template <class T> bool hasAlias(const T & alias);
            
        private:
            // Reference to derived connection instance
            SnmpConnection& m_snmpConnection;
            // Pointer to derived IO service instance
            SnmpIOServicePointer m_snmpIOService;

            ErrorHandler m_errorHandler;
            std::string m_hostname;
            unsigned int m_port;
            bool m_debug;
            long m_version;
            std::string m_community;
            snmp_session* m_session;
            AliasFlag m_flag;
            netsnmp_pdu* m_pdu;
            oid m_name[MAX_OID_LEN];
            size_t m_namelen;
            oid m_endoid[MAX_OID_LEN];
            size_t m_endlen;
            karabo::util::Hash m_output;
            std::string m_command;
            ReadVectorHashHandler m_complete;
        };

    }
}

#endif	/* KARABO_NET_SNMPCHANNEL_HH */

