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
 * File:   MetaTools.hh
 * Author: heisenb
 *
 * Created on August 10, 2016, 3:56 PM
 */

#ifndef KARABO_UTIL_METATOOLS_HH
#define KARABO_UTIL_METATOOLS_HH

#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>


namespace karabo {
    namespace util {

        class Hash;

        template <class T>
        struct is_shared_ptr : std::false_type {};

        template <class T>
        struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

        /**
         * Conditionally cast a type to Hash, if Hash is a base class, but
         * disallow this for shared pointers of these types. Will result in
         * a compiler error if this is attempted.
         *
         * Any type not derived from Hash is simply returned as is.
         */
        template <typename is_hash_base>
        struct conditional_hash_cast {
            // Here is for T deriving from Hash or Hash itself,
            // below is the specialisation for is_hash_base = std::false_type.

            template <typename T>
            static const Hash& cast(const T& v) {
                return reinterpret_cast<const Hash&>(v);
            }

            template <typename T>
            static Hash&& cast(T&& v) {
                // Following line works fine in Ubuntu20 (gcc9.3), but does not compile on CentOS7gcc7 and Ubuntu18
                // (also gcc7) with "invalid cast of an rvalue expression of type ‘karabo::util::NDArray’ to type
                // ‘karabo::util::Hash&&’" from 'Hash::set(somekey, NDArray(..)) in PropertyTest::writeOutput(..).
                // return reinterpret_cast<Hash&&> (std::forward<T>(v));
                T&& vTemp = static_cast<T&&>(std::forward<T>(v));
                return reinterpret_cast<Hash&&>(vTemp);
            }

            template <typename T>
            static Hash& cast(T& v) {
                return reinterpret_cast<Hash&>(v);
            }

            static Hash& cast(Hash& v) {
                return v;
            }

            static Hash&& cast(Hash&& v) {
                return std::move(v);
            }

            static const Hash& cast(const Hash& v) {
                return v;
            }

            template <typename T>
            static const std::shared_ptr<T> cast(const std::shared_ptr<T>& v) {
                // if the compiler ever reaches this point compilation is to fail on purpose, as
                // we only support explicit setting of Hash::Pointer to the Hash

                // is_hash_base: will always be std::true_type when dealing
                // with types derived from Hash, i.e in the context of this
                // template method evaluation.
                static_assert(std::is_same<is_hash_base, std::false_type>::value, // this evaluates false
                              "Inserting derived hash classes as pointers is not supported");
                return v;
            }
        };


        template <>
        struct conditional_hash_cast<std::false_type> {
            template <typename T>
            static T&& cast(T&& v) {
                return std::forward<T>(v);
            }
        };

        /**
         * Helper functions to compile-time distinguish if a dynamic_cast is needed.
         */

        // if it is not a virtual base but is a direct base we can static cast

        template <typename, typename>
        struct static_or_dyn_cast {
            template <typename T>
            static std::shared_ptr<T> cast(T* p) {
                return std::static_pointer_cast<T>(p->shared_from_this());
            }
        };

        // if it is a virtual base a dynamic cast must be made

        template <>
        struct static_or_dyn_cast<std::true_type, std::true_type> {
            template <typename T>
            static std::shared_ptr<T> cast(T* p) {
                return std::dynamic_pointer_cast<T>(p->shared_from_this());
            }
        };


        // First, a type trait to check whether a type can be static_casted to another
        template <typename From, typename To, typename = void>
        struct can_static_cast : std::false_type {};

        template <typename From, typename To>
        struct can_static_cast<From, To, std::void_t<decltype(static_cast<To>(std::declval<From>()))>>
            : std::true_type {};

        // Then, we apply the fact that a virtual base is first and foremost a base,
        // that, however, cannot be static_casted to its derived class.
        template <typename Base, typename Derived>
        struct is_virtual_base_of
            : std::conjunction<std::is_base_of<Base, Derived>, std::negation<can_static_cast<Base*, Derived*>>> {};

        // if this is not a direct base a dynamic cast must be made

        template <>
        struct static_or_dyn_cast<std::false_type, std::false_type> {
            template <typename T>
            static std::shared_ptr<T> cast(T* p) {
                return std::dynamic_pointer_cast<T>(p->shared_from_this());
            }
        };

        template <typename>
        struct cond_dyn_cast {
            template <typename T>
            static std::shared_ptr<T> cast(T* p) {
                return p->shared_from_this();
            }
        };

        template <>
        struct cond_dyn_cast<std::false_type> {
            template <typename T>
            static std::shared_ptr<T> cast(T* p) {
                typedef typename std::remove_reference<decltype(*(p->shared_from_this()))>::type BaseType;
                typedef typename is_virtual_base_of<BaseType, T>::type is_virtual_base;
                typedef typename std::is_base_of<BaseType, T>::type is_base;
                return static_or_dyn_cast<is_virtual_base, is_base>::cast(p);
            }
        };

        /**
         * Provides a wrapper with the same signature as f, but securing shared ownership of an object of type Obj
         * before execution of f.
         * Class Obj needs to derive somewhere in its inheritance tree from enable_shared_from_this();
         * @param f: a const member function, can have any argument types and any return value
         * @param o: a pointer to an object that has a member function f
         * @return a wrapped version of f.
         */
        template <typename Ret, typename... Args, typename Obj>
        std::function<Ret(Args...)> exec_weak_impl(Ret (Obj::*f)(Args...) const, const Obj* o) {
            typedef typename std::is_same<std::shared_ptr<Obj>, decltype(o->shared_from_this())>::type is_same_type;
            std::weak_ptr<const Obj> wp(cond_dyn_cast<is_same_type>::template cast(o));
            // we need to copy-capture here -> otherwise segfault, because f and wp go out of scope
            auto wrapped = [f, wp](Args... fargs) -> Ret {
                auto ptr = wp.lock();
                if (ptr) {
                    return (*ptr.*f)(fargs...);
                } else {
                    return Ret();
                }
            };
            return wrapped;
        }

        /**
         * Provides a wrapper with the same signature as f, but securing shared ownership of an object of type Obj
         * before execution of f.
         * Class Obj needs to derive somewhere in its inheritance tree from enable_shared_from_this();
         * @param f: a non-const member function, can have any argument types and any return value
         * @param o: a pointer to an object that has a member function f
         * @return a wrapped version of f.
         */
        template <typename Ret, typename... Args, typename Obj>
        std::function<Ret(Args...)> exec_weak_impl(Ret (Obj::*f)(Args...), Obj* o) {
            // Just cast to a const member function and re-use implementation for that.
            // Does anybody know how to do that with a C++ style cast for member function pointers?
            auto fConst = (Ret(Obj::*)(Args...) const)f;
            // No need for 'auto oConst = const_cast<const Obj*>(o);' since the above const cast ensures that the
            // correct exec_weak_impl is used -  a non const pointer can well be used in const context!
            return exec_weak_impl(fConst, o);
        }

        /**
         * Weakly binds member function f to an object of class Obj, but assures shared ownership of the object while f
         * is executed. This means that during the lifetime of calling f, the object cannot be destroyed, but
         * destruction is not blocked if f is not being executed but only bound.
         * Class Obj needs to derive from std::enable_shared_from_this and the object pointer o has to be held by a
         * shared_ptr. This means that you cannot use bind_weak within the constructor of Obj nor for objects
         * constructed on the stack.
         * Note that f may have any default constructable return type: If the bound functor will be called when the
         * object is already destroyed, the functor returns a default constructed object of the return type.
         *
         * Below is an example of how to bind to a boost::asio interface.
         *
         * void Device::executeStepFunction(int arg, const boost::system::error_code& error) {
         *     ....
         *     m_timer.async_wait(bind_weak(&Device::executeStepFunction, this, arg + 1, _1));
         *     ....
         * }
         *
         * @param f: function to bind, give as &Foo::bar
         * @param o: pointer to object to bind to
         * @param p: parameters as one would give to std::bind. Placeholders are fully supported.
         * @return: bound functor, compatible with boost bind.
         */
        template <typename F, typename Obj, typename... P>
        auto bind_weak(const F& f, Obj* const o, const P... p) -> decltype(std::bind(exec_weak_impl(f, o), p...)) {
            // note that std::arg<N>s cannot be forwarded, thus we work with references here.
            auto wrapped = exec_weak_impl(f, o);
            return std::bind(wrapped, p...);
        }

        // implementation details, users never invoke these directly
        namespace detail {

            // The code below was formulated as an answer to StackOverflow and can be read here:
            // http://stackoverflow.com/questions/10766112/c11-i-can-go-from-multiple-args-to-tuple-but-can-i-go-from-tuple-to-multiple

            template <typename F, typename Tuple, bool Done, int Total, int... N>
            struct call_impl {
                static void call(F f, Tuple&& t) {
                    call_impl<F, Tuple, Total == 1 + sizeof...(N), Total, N..., sizeof...(N)>::call(
                          f, std::forward<Tuple>(t));
                }
            };

            template <typename F, typename Tuple, int Total, int... N>
            struct call_impl<F, Tuple, true, Total, N...> {
                static void call(F f, Tuple&& t) {
                    f(std::get<N>(std::forward<Tuple>(t))...);
                }
            };
        } // namespace detail

        /**
         * Call a function f with arguments unpacked from a std::tuple
         * @param f
         * @param t
         */
        template <typename F, typename Tuple>
        void call(F f, Tuple&& t) {
            typedef typename std::decay<Tuple>::type ttype;
            detail::call_impl<F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(
                  f, std::forward<Tuple>(t));
        }
    } // namespace util
} // namespace karabo


#endif /* METATOOLS_HH */
