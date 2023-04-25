/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#ifndef KARABO_UTIL_PACKPARAMETERS
#define KARABO_UTIL_PACKPARAMETERS

namespace karabo {
    namespace util {
        class Hash;


        // implementation details, users never invoke these directly
        namespace detail {

            inline void unpack_r(const Hash& hash, char i) {}

            template <class Tfirst, class... Trest>
            inline void unpack_r(const Hash& hash, char i, Tfirst& first, Trest&... rest) {
                // string has length 4 for better alignment
                // (tested that, makes a difference in assembly)
                char name[4] = "a ";
                name[1] = i;
                first = hash.get<Tfirst>(name);
                detail::unpack_r(hash, i + 1, rest...);
            }
        } // namespace detail

        // implementation details, users never invoke these directly
        namespace detail {

            template <char C, typename... Args>
            struct unpack_impl;

            template <char C, typename A, typename... Args>
            struct unpack_impl<C, A, Args...> {
                static auto unpack(const karabo::util::Hash& h)
                      -> decltype(std::tuple_cat(std::tie(h.get<A>(std::string())),
                                                 unpack_impl<C + 1, Args...>::unpack(h))) {
                    constexpr const char key[] = {'a', C, '\0', '\0'}; // string has length 4 for better alignment
                    return std::tuple_cat(std::tie(h.get<A>(key)), unpack_impl<C + 1, Args...>::unpack(h));
                }
            };

            template <char C, typename A>
            struct unpack_impl<C, A> {
                static auto unpack(const karabo::util::Hash& h) -> decltype(std::tie(h.get<A>(std::string()))) {
                    constexpr const char key[] = {'a', C, '\0', '\0'}; // string has length 4 for better alignment
                    return std::tie(h.get<A>(key));
                }
            };

            template <char C>
            struct unpack_impl<C> {
                static std::tuple<> unpack(const karabo::util::Hash& h) {
                    return std::tuple<>();
                }
            };
        } // namespace detail

        // implementation details, users never invoke these directly
        namespace detail {

            inline void pack_r(Hash& hash, char i) {}

            template <class Tfirst, class... Trest>
            inline void pack_r(Hash& hash, char i, const Tfirst& first, const Trest&... rest) {
                char name[4] = "a ";
                name[1] = i;
                hash.set(name, first);
                detail::pack_r(hash, i + 1, rest...);
            }
        } // namespace detail

        /**
         * Unpack the hash (typically coming from the network) into the
         * parameters given by reference.
         */
        template <class... Ts>
        inline void unpack(const Hash& hash, Ts&... args) {
            detail::unpack_r(hash, '1', args...);
        }

        /**
         * Unpack parameters into a tuple holding only references
         * @param h Hash with keys a1, a2, etc. encoding function arguments
         * @return std::tuple<Args&...>
         */
        template <typename... Args>
        auto unpack(const karabo::util::Hash& h) -> decltype(detail::unpack_impl<'1', Args...>::unpack(h)) {
            return detail::unpack_impl<'1', Args...>::unpack(h);
        }

        /**
         * Pack the parameters into a hash for transport over the network.
         * @param hash Will be filled with keys a1, a2, etc. and associated values
         * @param args Any type and number of arguments to associated to hash keys
         */
        template <class... Ts>
        inline void pack(Hash& hash, const Ts&... args) {
            detail::pack_r(hash, '1', args...);
        }
    } // namespace util
} // namespace karabo

#endif
