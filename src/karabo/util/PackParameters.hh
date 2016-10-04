#ifndef KARABO_UTIL_PACKPARAMETERS
#define KARABO_UTIL_PACKPARAMETERS

namespace karabo {
    namespace util {
        class Hash;

        inline void unpack_r(const Hash& hash, char i) {
        }

        template <class Tfirst, class ... Trest>
        inline void unpack_r(const Hash& hash, char i, Tfirst &first, Trest& ... rest) {
            char name[4] = "a ";
            name[1] = i;
            first = hash.get<Tfirst>(name);
            unpack_r(hash, i + 1, rest...);
        }

        template <class ... Ts>
        inline void unpack(const Hash& hash, Ts & ... args) {
            unpack_r(hash, '1', args...);
        }

        inline void pack_r(Hash& hash, char i) {
        }


        template <class Tfirst, class ... Trest>
        inline void pack_r(Hash& hash, char i, const Tfirst& first, const Trest & ... rest) {
            char name[4] = "a ";
            name[1] = i;
            hash.set(name, first);
            pack_r(hash, i + 1, rest...);
        }

        template <class ... Ts>
        inline void pack(Hash& hash, const Ts& ... args) {
            pack_r(hash, '1', args...);
        }
    }
}

#endif
