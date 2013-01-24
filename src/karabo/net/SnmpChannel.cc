/* 
 * File:   SnmpChannel.cc
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on July 14, 2011, 5:54 PM
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
//#include <karabo/io/Reader.hh>
//#include <karabo/io/Writer.hh>

#include "SnmpChannel.hh"
#include "SnmpConnection.hh"
#include "SnmpIOService.hh"

namespace karabo {
    namespace net {

        using namespace std;
        using namespace karabo::util;

        //        static string oidToString(const oid* name, size_t namelen);

        SnmpChannel::SnmpChannel(SnmpConnection& connection, const std::string& hostname,
                unsigned int port, long version, const std::string& community) :
        Channel(connection), m_snmpConnection(connection), m_hostname(hostname), m_port(port), m_debug(false), m_version(version),
        m_community(community), m_session(0) {
            // Retrieve derived IO service instance by templated casting
            m_snmpIOService = connection.getIOService()->castTo<SnmpIOService > ();
            ::snmp_session session;
            ::snmp_sess_init(&session);
            session.version = m_version;
            session.peername = strdup(m_hostname.c_str());
            session.community = (unsigned char*) strdup(m_community.c_str());
            session.community_len = m_community.size();
            //            session.callback = async_snmp_callback;
            //            session.callback_magic = this;
            m_session = ::snmp_open(&session);
            if (!m_session) {
                throw KARABO_IO_EXCEPTION("SNMP Session for " + m_hostname + " with " + m_community + " failed to open -- " + snmp_api_errstring(::snmp_errno));
            }
            if (m_snmpConnection.m_aliasMode == "Schema")
                m_flag = Schema;
            else if (m_snmpConnection.m_aliasMode == "SchemaOnly")
                m_flag = SchemaOnly;
            else if (m_snmpConnection.m_aliasMode == "SchemaOnlyWithException")
                m_flag = SchemaOnlyWithException;
            else
                m_flag = NoSchema;
        }

        SnmpChannel::~SnmpChannel() {
        }

        template <class T>
        const T& SnmpChannel::key2alias(const std::string& key) {
            if (m_snmpConnection.m_schema.hasKey(key)) {
                return m_snmpConnection.m_schema.key2alias<T > (key);
            } else {
                throw KARABO_PARAMETER_EXCEPTION("\"" + key + "\" is not a valid variable");
            }
        }

        template <class T>
        const std::string& SnmpChannel::alias2key(const T& alias) {
            if (m_snmpConnection.m_schema.hasAlias(alias)) {
                return m_snmpConnection.m_schema.alias2key(alias);
            } else {
                throw KARABO_PARAMETER_EXCEPTION("The provided alias is not valid.");
            }
        }

        bool SnmpChannel::hasKey(const std::string& key) {
            return m_snmpConnection.m_schema.hasKey(key);
        }

        bool SnmpChannel::keyHasAlias(const std::string& key) {
            return m_snmpConnection.m_schema.keyHasAlias(key);
        }

        template <class T>
        bool SnmpChannel::hasAlias(const T & alias) {
            return m_snmpConnection.m_schema.hasAlias(alias);
        }

        void SnmpChannel::setErrorHandler(const ErrorHandler& handler) {
            m_errorHandler = handler;
        }

        void SnmpChannel::requestMoreDataAsync() {
            snmp_callback cb;

            string command = m_command;
            if (command == "GET") {
                cb = async_snmpget_callback;
                m_pdu = snmp_pdu_create(SNMP_MSG_GET);
                snmp_add_null_var(m_pdu, m_name, m_namelen);
            } else if (command == "GETNEXT") {
                cb = async_snmpget_callback;
                m_pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
                snmp_add_null_var(m_pdu, m_name, m_namelen);
            } else if (command == "WALK") {
                cb = async_snmpwalk_callback;
                m_pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
                snmp_add_null_var(m_pdu, m_name, m_namelen);
            } else if (command == "GETBULK") {
                cb = async_snmpget_callback;
                m_pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
                m_pdu->non_repeaters = 0;
                m_pdu->max_repetitions = 10;
                snmp_add_null_var(m_pdu, m_name, m_namelen);
            } else if (command == "WALKBULK") {
                cb = async_snmpwalkbulk_callback;
                m_pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
                m_pdu->non_repeaters = 0;
                m_pdu->max_repetitions = 10;
                snmp_add_null_var(m_pdu, m_name, m_namelen);
            } else
                return;

            if (snmp_async_send(m_session, m_pdu, cb, this))
                return;
            else {
                cout << "requestMoreData : snmp_send failed -- " << snmp_api_errstring(snmp_errno) << endl;
                snmp_free_pdu(m_pdu);
                m_pdu = 0;
            }
        }

        void SnmpChannel::convertVarbindToHash(netsnmp_variable_list* v, Hash & hash) {

            if (m_debug) print_variable(v->name, v->name_length, v);

            stringstream ss;
            for (unsigned int i = 0; i < v->name_length; i++) {
                ss << '.' << v->name[i];
            }
            string sname(ss.str());
            // Error: sometimes v->type == ASN_OCTET_STR. Should be ASN_BIT_STR
            // Workaround: use type calculations via mib tree
            tree* head_node = get_tree_head();
            tree* var_node = get_tree(v->name, v->name_length, head_node);
            int vtype = var_node->type == TYPE_BITSTRING ? mib_to_asn_type(var_node->type) : v->type;

            switch (vtype) {
                case ASN_INTEGER:
                {
                    int ival = *v->val.integer;
                    if (m_debug) cout << sname << " = " << "INTEGER : " << ival << endl;
                    hash.set(sname, ival);
                    break;
                }
                case ASN_OCTET_STR:
                {
                    string sval((char*) v->val.string, v->val_len);
                    if (m_debug) cout << sname << " = " << "STRING : " << sval << endl;
                    hash.set(sname, sval);
                    break;
                }
                case ASN_BIT_STR:
                {
                    unsigned char* ch = v->val.bitstring;
                    size_t vlen = v->val_len;
                    deque<bool> vec;
                    if (m_debug) cout << sname << " = " << "BITS : ";
                    for (size_t l = 0; l < vlen; l++) {
                        for (int bit = 0; bit < 8; bit++) {
                            if (ch[l] & (0x80 >> bit)) {
                                vec.push_back(true);
                                if (m_debug) cout << '1';
                            } else {
                                vec.push_back(false);
                                if (m_debug) cout << '0';
                            }
                        }
                    }
                    if (m_debug) cout << endl;
                    hash.set(sname, vec);
                    break;
                }
                case ASN_OPAQUE:
                {
                    stringstream ss;
                    ss.clear();
                    for (size_t i = 0; i < v->val_len; i++)
                        ss << ' ' << hex << v->val.string[i];
                    string sval(ss.str());
                    if (m_debug) cout << sname << " = " << "OPAQUE : 0x" << sval << endl;
                    hash.set(sname, sval);
                    break;
                }
                case ASN_OBJECT_ID:
                {
                    stringstream ss;
                    for (size_t i = 0; i < v->val_len / sizeof (oid); i++) {
                        ss << '.' << v->val.objid[i];
                    }
                    string sval(ss.str());
                    if (m_debug) cout << sname << " = " << "OID : " << sval << endl;
                    hash.set(sname, sval);
                    break;
                }
                case ASN_TIMETICKS:
                {
                    unsigned int ticks = *(unsigned int*) v->val.integer;
                    if (m_debug) cout << "Timeticks : " << ticks << endl;
                    hash.set(sname, ticks);
                    break;
                }
                case ASN_GAUGE:
                {
                    unsigned int gauge = *(unsigned int*) v->val.integer;
                    if (m_debug) cout << sname << " = " << "Gauge32 : " << gauge << endl;
                    hash.set(sname, gauge);
                    break;
                }
                case ASN_COUNTER:
                {
                    unsigned int counter = *(unsigned int*) v->val.integer;
                    if (m_debug) cout << sname << " = " << "Counter32 : " << counter << endl;
                    hash.set(sname, counter);
                    break;
                }
                case ASN_IPADDRESS:
                {
                    unsigned char* ip = v->val.string;
                    vector<char> vec;
                    if (m_debug)
                        cout << sname << " = " << "IpAddress : "
                            << ip[0] << '.' << ip[1] << '.' << ip[2] << '.' << ip[3] << endl;
                    for (int i = 0; i < 4; i++) vec.push_back(ip[i]);
                    hash.set(sname, vec);
                    break;
                }
                case ASN_NULL:
                    if (m_debug) cout << sname << " = " << "NULL" << endl;
                    hash.set(sname, 0);
                    break;
                case ASN_UINTEGER:
                {
                    unsigned int u = *(unsigned int*) v->val.integer;
                    if (m_debug) cout << sname << " = " << "UINTEGER : " << u << endl;
                    hash.set(sname, u);
                    break;
                }
                case ASN_COUNTER64:
                case ASN_OPAQUE_U64:
                case ASN_OPAQUE_I64:
                case ASN_OPAQUE_COUNTER64:
                {
                    unsigned long long ull = *(unsigned long long*) v->val.counter64;
                    if (m_debug) cout << sname << " = " << "COUNTER64 : " << ull << endl;
                    hash.set(sname, ull);
                    break;
                }
                case ASN_OPAQUE_FLOAT:
                {
                    float f = *(float*) v->val.floatVal;
                    if (m_debug) cout << sname << " = " << "FLOAT : " << f << endl;
                    hash.set(sname, f);
                    break;
                }
                case ASN_OPAQUE_DOUBLE:
                {
                    double d = *(double*) v->val.doubleVal;
                    if (m_debug) cout << sname << " = " << "DOUBLE : " << d << endl;
                    hash.set(sname, d);
                    break;
                }
                default:
                    break;
            }
        }

        int SnmpChannel::async_snmpget_callback(int operation, snmp_session *sp, int reqid, snmp_pdu *response, void *cbdata) {
            // cast callback data to channel pointer
            SnmpChannel* chn = static_cast<SnmpChannel*> (cbdata);
            ostringstream err;

            if (operation == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE) {
                if (response->errstat == SNMP_ERR_NOERROR) {
                    for (netsnmp_variable_list* v = response->variables; v; v = v->next_variable)
                        chn->convertVarbindToHash(v, chn->m_output);
                } else {
                    // error processing ...
                    err << "Error in packet:\nReason: " << snmp_errstring(response->errstat);
                    if (response->errindex != 0) {
                        netsnmp_variable_list * v = response->variables;
                        err << "\nFailed object: ";
                        for (int count = 1; v && count != response->errindex; v = v->next_variable, count++);
                        if (v) {
                            char buf[256];
                            snprint_objid(buf, 256, v->name, v->name_length);
                            err << buf;
                        }
                    }
                    chn->m_errorHandler(chn->channel(), err.str());
                }
            } else {
                err << "Timeout: No Response from " << sp->peername; // timeout
                chn->m_errorHandler(chn->channel(), err.str());
            }

            chn->m_snmpIOService->decreaseReplyCount();
            vector<char> vecCommand(chn->m_command.begin(), chn->m_command.end());
            chn->m_complete(chn->channel(), vecCommand, chn->m_output);

            return 1;
        }

        int SnmpChannel::async_snmpwalk_callback(int operation, snmp_session *sp, int reqid, snmp_pdu *response, void *cbdata) {
            SnmpChannel* chn = static_cast<SnmpChannel*> (cbdata);
            ostringstream err;

            if (operation == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE) {
                if (response->errstat == SNMP_ERR_NOERROR) {
                    netsnmp_variable_list* v = response->variables;

                    for (; v; v = v->next_variable) {
                        if (snmp_oid_compare(chn->m_endoid, chn->m_endlen, v->name, v->name_length) <= 0)
                            continue; // not part of this tree

                        chn->convertVarbindToHash(v, chn->m_output);

                        if (v->type != SNMP_ENDOFMIBVIEW && v->type != SNMP_NOSUCHOBJECT && v->type != SNMP_NOSUCHINSTANCE) {
                            // no exceptions
                            // check that OID increasing
                            if (snmp_oid_compare(chn->m_name, chn->m_namelen, v->name, v->name_length) >= 0) {
                                err << "Error: OID not increasing: ";
                                {
                                    char buf[256];
                                    snprint_objid(buf, 256, chn->m_name, chn->m_namelen);
                                    err << buf << " >= ";
                                    snprint_objid(buf, 256, v->name, v->name_length);
                                    err << buf;
                                }
                                chn->m_errorHandler(chn->channel(), err.str());
                            }
                            memmove(chn->m_name, v->name, v->name_length * sizeof (oid));
                            chn->m_namelen = v->name_length;
                            chn->requestMoreDataAsync(); // next request...
                            return 1;
                        } else
                            break; // EndOfMib || NoSuchObject || NoSuchInstance
                    }
                } else {
                    if (response->errstat == SNMP_ERR_NOSUCHNAME)
                        err << "End of MIB";
                    else {
                        err << "Error in packet:\nReason: " << snmp_errstring(response->errstat) << "\n";
                        if (response->errindex != 0) {
                            netsnmp_variable_list * v = response->variables;
                            err << "Failed object: ";
                            for (int count = 1; v && count != response->errindex; v = v->next_variable, count++);
                            if (v) {
                                char buf[256];
                                snprint_objid(buf, 256, v->name, v->name_length);
                                err << buf;
                            }
                        }
                    }
                    chn->m_errorHandler(chn->channel(), err.str());
                }
            } else {
                cout << "Timeout: No Response from " << sp->peername; // timeout
                chn->m_errorHandler(chn->channel(), err.str());
            }

            chn->m_snmpIOService->decreaseReplyCount();
            vector<char> vecCommand(chn->m_command.begin(), chn->m_command.end());
            chn->m_complete(chn->channel(), vecCommand, chn->m_output);

            return 1;
        }

        int SnmpChannel::async_snmpwalkbulk_callback(int operation, snmp_session *sp, int reqid, snmp_pdu *response, void *data) {
            SnmpChannel* chn = static_cast<SnmpChannel*> (data);
            ostringstream err;

            if (operation == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE) {
                if (response->errstat == SNMP_ERR_NOERROR) {
                    netsnmp_variable_list* v = response->variables;

                    for (; v; v = v->next_variable) {
                        if (v->name_length < chn->m_endlen || memcmp(chn->m_endoid, v->name, chn->m_endlen * sizeof (oid)) != 0)
                            continue; // not part of this tree

                        chn->convertVarbindToHash(v, chn->m_output);

                        if (v->type != SNMP_ENDOFMIBVIEW && v->type != SNMP_NOSUCHOBJECT && v->type != SNMP_NOSUCHINSTANCE) {
                            // no exceptions
                            // check that OID increasing
                            if (snmp_oid_compare(chn->m_name, chn->m_namelen, v->name, v->name_length) >= 0) {
                                err << "Error: OID not increasing: ";
                                {
                                    char buf[256];
                                    snprint_objid(buf, 256, chn->m_name, chn->m_namelen);
                                    err << buf << " >= ";
                                    snprint_objid(buf, 256, v->name, v->name_length);
                                    err << buf;
                                }
                                chn->m_errorHandler(chn->channel(), err.str());
                            }
                            if (!v->next_variable) {
                                memmove(chn->m_name, v->name, v->name_length * sizeof (oid));
                                chn->m_namelen = v->name_length;
                                chn->requestMoreDataAsync(); // to be continued...
                                return 1;
                            }
                        } else
                            break; // EndOfMib || NoSuchObject || NoSuchInstance
                    }
                } else {
                    if (response->errstat == SNMP_ERR_NOSUCHNAME)
                        err << "End of MIB";
                    else {
                        err << "Error in packet:\nReason: " << snmp_errstring(response->errstat) << "\n";
                        if (response->errindex != 0) {
                            netsnmp_variable_list * v = response->variables;
                            err << "Failed object: ";
                            for (int count = 1; v && count != response->errindex; v = v->next_variable, count++);
                            if (v) {
                                char buf[256];
                                snprint_objid(buf, 256, v->name, v->name_length);
                                err << buf;
                            }
                        }
                    }
                    chn->m_errorHandler(chn->channel(), err.str());
                }
            } else {
                cout << "Timeout: No Response from " << sp->peername; // timeout
                chn->m_errorHandler(chn->channel(), err.str());
            }

            chn->m_snmpIOService->decreaseReplyCount();
            vector<char> vecCommand(chn->m_command.begin(), chn->m_command.end());
            chn->m_complete(chn->channel(), vecCommand, chn->m_output);

            return 1;
        }

        void SnmpChannel::read(std::vector<char>& cmd, Hash & hash) {
            cmd.assign(m_command.begin(), m_command.end());
            read(hash);
        }

        void SnmpChannel::read(string& command, Hash& hash) {
            command = m_command;
            read(hash);
        }

        void SnmpChannel::read(Hash& output) {
            Hash hash;
            if (m_command == "WALK")
                read_snmpwalk(hash);
            else if (m_command == "WALKBULK")
                read_snmpwalkbulk(hash);
            else
                read_snmpget(hash);
            
            for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                try {

                    switch (m_flag) {

                        case Schema:
                            if (hasAlias<string > (it->first)) output[alias2key<string > (it->first)] = it->second;
                            else output[it->first] = it->second;
                            break;

                        case SchemaOnly:
                            if (hasAlias<string > (it->first)) output[alias2key<string > (it->first)] = it->second;
                            break;

                        case SchemaOnlyWithException:
                            output[alias2key<string > (it->first)] = it->second;
                            break;

                        default:
                            output[it->first] = it->second;
                    }

                } catch (...) {
                    // ignore this alias
                }
            }
        }

        void SnmpChannel::read_snmpget(Hash& hash) {
            netsnmp_pdu* response;
            netsnmp_variable_list *v;
            ostringstream err;

            int status = snmp_synch_response(m_session, m_pdu, &response);
            if (status == STAT_SUCCESS) {
                if (response->errstat == SNMP_ERR_NOERROR)
                    for (v = response->variables; v; v = v->next_variable)
                        convertVarbindToHash(v, hash);
                else {
                    err << "Error in packet.\nReason: " << snmp_errstring(response->errstat);
                    if (response->errindex != 0) {
                        int idx;
                        err << "\nFailed object: ";
                        for (idx = 1, v = response->variables;
                                v && (idx != response->errindex);
                                v = v->next_variable, idx++);
                        if (v) {
                            char buf[256];
                            snprint_objid(buf, 256, v->name, v->name_length);
                            err << buf;
                        }
                    }
                    m_errorHandler(channel(), err.str());
                }
            } else if (status == STAT_TIMEOUT) {
                err << "Timeout: No Response from " << m_session->peername;
                m_errorHandler(channel(), err.str());
            } else { /* status == STAT_ERROR */
                err << "SnmpChannel::read -- " << snmp_api_errstring(snmp_errno);
                m_errorHandler(channel(), err.str());
            }

            if (response)
                snmp_free_pdu(response);
        }

        void SnmpChannel::read_snmpwalk(Hash& hash) {
            netsnmp_pdu* response;
            netsnmp_variable_list *v;
            bool running = true;

            while (running) {
                ostringstream err;


                int status = snmp_synch_response(m_session, m_pdu, &response);
                if (status == STAT_SUCCESS) {
                    if (response->errstat == SNMP_ERR_NOERROR) {
                        for (v = response->variables; v; v = v->next_variable) {
                            if (snmp_oid_compare(m_endoid, m_endlen, v->name, v->name_length) <= 0) {
                                running = false; // not part of this subtree
                                continue;
                            }

                            convertVarbindToHash(v, hash);

                            if (v->type != SNMP_ENDOFMIBVIEW && v->type != SNMP_NOSUCHOBJECT && v->type != SNMP_NOSUCHINSTANCE) {
                                /*
                                 * not an exception value 
                                 */
                                if (snmp_oid_compare(m_name, m_namelen, v->name, v->name_length) >= 0) {
                                    char buf[256];
                                    err << "Error: OID not increasing: ";
                                    snprint_objid(buf, 256, m_name, m_namelen);
                                    err << buf << " >= ";
                                    snprint_objid(buf, 256, v->name, v->name_length);
                                    err << buf;
                                    m_errorHandler(channel(), err.str());
                                    running = false;
                                }
                                memmove((char *) m_name, (char *) v->name, v->name_length * sizeof (oid));
                                m_namelen = v->name_length;
                                m_pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
                                snmp_add_null_var(m_pdu, m_name, m_namelen);
                            } else
                                /*
                                 * an exception value, so stop 
                                 */
                                running = false;
                        }
                    } else {
                        running = false;
                        if (response->errstat == SNMP_ERR_NOSUCHNAME) {
                            err << "End of MIB";
                        } else {
                            err << "Error in packet.\nReason: " << snmp_errstring(response->errstat);
                            if (response->errindex != 0) {
                                int idx;
                                err << "\nFailed object: ";
                                for (idx = 1, v = response->variables;
                                        v && (idx != response->errindex);
                                        v = v->next_variable, idx++);
                                if (v) {
                                    char buf[256];
                                    snprint_objid(buf, 256, v->name, v->name_length);
                                    err << buf;
                                }
                            }
                        }
                        m_errorHandler(channel(), err.str());
                    }
                } else if (status == STAT_TIMEOUT) {
                    err << "Timeout: No Response from " << m_session->peername;
                    m_errorHandler(channel(), err.str());
                    running = false;
                } else { /* status == STAT_ERROR */
                    err << "SnmpChannel::read -- " << snmp_api_errstring(snmp_errno);
                    m_errorHandler(channel(), err.str());
                    running = false;
                }
            }
            if (response)
                snmp_free_pdu(response);
        }

        void SnmpChannel::read_snmpwalkbulk(Hash& hash) {
            netsnmp_pdu* response;
            netsnmp_variable_list *v;
            bool running = true;

            while (running) {
                ostringstream err;


                int status = snmp_synch_response(m_session, m_pdu, &response);
                if (status == STAT_SUCCESS) {
                    if (response->errstat == SNMP_ERR_NOERROR) {
                        for (v = response->variables; v; v = v->next_variable) {
                            if (v->name_length < m_endlen || memcmp(m_endoid, v->name, m_endlen * sizeof (oid)) != 0) {
                                running = false; // not part of this subtree
                                continue;
                            }

                            convertVarbindToHash(v, hash);

                            if (v->type != SNMP_ENDOFMIBVIEW && v->type != SNMP_NOSUCHOBJECT && v->type != SNMP_NOSUCHINSTANCE) {
                                /*
                                 * not an exception value 
                                 */
                                if (snmp_oid_compare(m_name, m_namelen, v->name, v->name_length) >= 0) {
                                    char buf[256];
                                    err << "Error: OID not increasing: ";
                                    snprint_objid(buf, 256, m_name, m_namelen);
                                    err << buf << " >= ";
                                    snprint_objid(buf, 256, v->name, v->name_length);
                                    err << buf;
                                    m_errorHandler(channel(), err.str());
                                    running = false;
                                }
                                if (v->next_variable == NULL) {
                                    memmove((char *) m_name, (char *) v->name, v->name_length * sizeof (oid));
                                    m_namelen = v->name_length;
                                    m_pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
                                    m_pdu->non_repeaters = 0;
                                    m_pdu->max_repetitions = 10;
                                    snmp_add_null_var(m_pdu, m_name, m_namelen);
                                }
                            } else
                                /*
                                 * an exception value, so stop 
                                 */
                                running = false;
                        }
                    } else {
                        running = false;
                        if (response->errstat == SNMP_ERR_NOSUCHNAME) {
                            err << "End of MIB";
                        } else {
                            err << "Error in packet.\nReason: " << snmp_errstring(response->errstat);
                            if (response->errindex != 0) {
                                int idx;
                                err << "\nFailed object: ";
                                for (idx = 1, v = response->variables;
                                        v && (idx != response->errindex);
                                        v = v->next_variable, idx++);
                                if (v) {
                                    char buf[256];
                                    snprint_objid(buf, 256, v->name, v->name_length);
                                    err << buf;
                                }
                            }
                        }
                        m_errorHandler(channel(), err.str());
                    }
                } else if (status == STAT_TIMEOUT) {
                    err << "Timeout: No Response from " << m_session->peername;
                    m_errorHandler(channel(), err.str());
                    running = false;
                } else { /* status == STAT_ERROR */
                    err << "SnmpChannel::read -- " << snmp_api_errstring(snmp_errno);
                    m_errorHandler(channel(), err.str());
                    running = false;
                }
            }
            if (response)
                snmp_free_pdu(response);
        }

        void SnmpChannel::readAsyncVectorHash(const ReadVectorHashHandler & handler) {
            snmp_callback cb;
            m_complete = handler; // store 'read handler' object

            string command(m_command);
            if (command == "WALK")
                cb = async_snmpwalk_callback;
            else if (command == "WALKBULK")
                cb = async_snmpwalkbulk_callback;
            else
                cb = async_snmpget_callback;

            if (snmp_async_send(m_session, m_pdu, cb, this)) {
                m_snmpIOService->increaseReplyCount();
            } else {
                snmp_free_pdu(m_pdu);
                throw KARABO_IO_EXCEPTION("snmp_send to " + m_hostname + " failed -- " + snmp_api_errstring(snmp_errno));
            }
        }

        void SnmpChannel::write(const vector<char>& cmd, const Hash& hash) {
            m_command.assign(cmd.begin(), cmd.end());
            write(m_command, hash);
        }

        void SnmpChannel::write(const Hash& hash) {
            m_command = "GET";
            write(m_command, hash);
        }

        void SnmpChannel::write(const string& cmd, const Hash& input) {
            Hash hash;
            try {
                for (Hash::const_iterator it = input.begin(); it != input.end(); ++it) {

                    switch (m_flag) {

                        case Schema:
                            if (keyHasAlias(it->first))
                                hash[key2alias<string > (it->first)] = it->second;
                            else
                                hash[it->first] = it->second;
                            break;

                        case SchemaOnly:
                            if (keyHasAlias(it->first))
                                hash[key2alias<string > (it->first)] = it->second;
                            break;

                        case SchemaOnlyWithException:
                            hash[alias2key<string > (it->first)] = it->second;
                            break;

                        default: // NoSchema
                            hash[it->first] = it->second;
                    }
                }
            } catch (...) {
                KARABO_RETHROW // all keys should have an alias
            }
            if (hash.empty())
                throw KARABO_PARAMETER_EXCEPTION("Empty payload for command " + cmd);

            m_command = cmd; // store command
            m_output.clear();

            if (m_command == "GET" || m_command == "GETNEXT" || m_command == "GETBULK") {

                if (m_command == "GET")
                    m_pdu = snmp_pdu_create(SNMP_MSG_GET);
                else if (m_command == "GETNEXT")
                    m_pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
                else {
                    m_pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
                    m_pdu->non_repeaters = 0;
                    m_pdu->max_repetitions = 10;
                }

                for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                    m_namelen = MAX_OID_LEN;
                    if (!snmp_parse_oid(it->first.c_str(), m_name, &m_namelen)) {
                        throw KARABO_PARAMETER_EXCEPTION("Invalid OID " + it->first + " -- " + snmp_api_errstring(snmp_errno));
                    } else {
                        snmp_add_null_var(m_pdu, m_name, m_namelen);
                    }
                }

            } else if (m_command == "SET") {

                m_pdu = snmp_pdu_create(SNMP_MSG_SET);
                for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {

                    const string& key = it->first;
                    m_namelen = MAX_OID_LEN;
                    if (!snmp_parse_oid(it->first.c_str(), m_name, &m_namelen)) {
                        throw KARABO_PARAMETER_EXCEPTION("Invalid OID " + key + " -- " + snmp_api_errstring(snmp_errno));
                    }

                    Types::ReferenceType type = hash.getTypeAsId(it);

                    switch (type) {

                        case Types::BOOL:
                        {
                            int i = hash.get<bool>(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_INTEGER, &i, sizeof (int));
                            break;
                        }
                        case Types::INT8:
                        {
                            int i = hash.get<char >(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_INTEGER, &i, sizeof (int));
                            break;
                        }
                        case Types::INT16:
                        {
                            int i = hash.get<short >(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_INTEGER, &i, sizeof (int));
                            break;
                        }
                        case Types::INT32:
                        {
                            int i = hash.get<int >(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_INTEGER, &i, sizeof (int));
                            break;
                        }
                        case Types::INT64:
                        {
                            long long i = hash.get<long long >(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_INTEGER64, &i, sizeof (long long));
                            break;
                        }
                        case Types::UINT8:
                        {
                            unsigned int u = hash.get<unsigned char >(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_UNSIGNED, &u, sizeof (unsigned int));
                            break;
                        }
                        case Types::UINT16:
                        {
                            unsigned int u = hash.get<unsigned short >(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_UNSIGNED, &u, sizeof (unsigned int));
                            break;
                        }
                        case Types::UINT32:
                        {
                            unsigned int u = hash.get<unsigned int >(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_UNSIGNED, &u, sizeof (unsigned int));
                            break;
                        }
                        case Types::UINT64:
                        {
                            unsigned long long u = hash.get<unsigned long long >(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_UNSIGNED64, &u, sizeof (unsigned long long));
                            break;
                        }
                        case Types::STRING:
                        {
                            string s = hash.get<string > (key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_OCTET_STR, s.c_str(), s.size());
                            break;
                        }
                        case Types::FLOAT:
                        {
                            float f = hash.get<float>(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_OPAQUE_FLOAT, &f, sizeof (float));
                            break;
                        }
                        case Types::DOUBLE:
                        {
                            double d = hash.get<double>(key);
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_OPAQUE_DOUBLE, &d, sizeof (double));
                            break;
                        }
                        case Types::VECTOR_BOOL:
                        {
                            deque<bool> vb = hash.get < deque<bool > >(key);
                            unsigned char *cp = new unsigned char[ (vb.size() + 7) / 8 ];
                            int cplen = (vb.size() + 7) / 8;
                            memset(cp, 0, cplen);
                            for (unsigned int i = 0; i < vb.size(); i++) {
                                unsigned int idx = i / 8;
                                unsigned int bit = i % 8;
                                if (vb[i]) cp[idx] |= (0x80 >> bit);
                            }
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_OCTET_STR, cp, cplen);
                            delete[] cp;
                            break;
                        }
                        case Types::VECTOR_UINT32:
                        case Types::VECTOR_INT32:
                        {
                            vector<int> vi = hash.get<vector<int > >(key);
                            int* ip = new int[ vi.size() ];
                            for (unsigned int i = 0; i < vi.size(); i++) ip[i] = vi[i];
                            snmp_pdu_add_variable(m_pdu, m_name, m_namelen, ASN_OBJECT_ID, ip, vi.size() * sizeof (oid));
                            delete[] ip;
                            break;
                        }
                        default:
                            throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' associated with SNMP invalid value!");
                    }
                }

            } else if (m_command == "WALK") {

                Hash::const_iterator it = hash.begin();
                m_namelen = MAX_OID_LEN;
                if (!snmp_parse_oid(it->first.c_str(), m_name, &m_namelen)) {
                    throw KARABO_PARAMETER_EXCEPTION("Invalid OID " + it->first + " -- " + snmp_api_errstring(snmp_errno));
                }
                memmove(m_endoid, m_name, m_namelen * sizeof (oid));
                m_endlen = m_namelen;
                m_endoid[m_endlen - 1]++;

                m_pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
                snmp_add_null_var(m_pdu, m_name, m_namelen);

            } else if (m_command == "WALKBULK") {

                Hash::const_iterator it = hash.begin();
                m_namelen = MAX_OID_LEN;
                if (!snmp_parse_oid(it->first.c_str(), m_name, &m_namelen)) {
                    throw KARABO_PARAMETER_EXCEPTION("Invalid OID " + it->first + " -- " + snmp_api_errstring(snmp_errno));
                }
                memmove(m_endoid, m_name, m_namelen * sizeof (oid));
                m_endlen = m_namelen;

                m_pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
                m_pdu->non_repeaters = 0;
                m_pdu->max_repetitions = 10;
                snmp_add_null_var(m_pdu, m_name, m_namelen);

            } else {
                throw KARABO_PARAMETER_EXCEPTION("Command '" + m_command + "' is not supported.");
            }
        }

        void SnmpChannel::writeAsyncVectorHash(const vector<char>& data, const Hash& header, const WriteCompleteHandler & handler) {
            write(data, header);
            handler(channel());
        }

        void SnmpChannel::writeAsyncStringHash(const string& data, const Hash& header, const WriteCompleteHandler & handler) {
            write(data, header);
            handler(channel());
        }

        void SnmpChannel::writeAsyncHash(const Hash& header, const WriteCompleteHandler & handler) {
            string data("GET");
            write(data, header);
            handler(channel());
        }

        void SnmpChannel::close() {
            unregisterChannel(shared_from_this());
        }
    }
}

