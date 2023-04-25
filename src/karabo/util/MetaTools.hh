/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   MetaTools.hh
 * Author: heisenb
 *
 * Created on August 10, 2016, 3:56 PM
 */

#ifndef KARABO_UTIL_METATOOLS_HH
#define KARABO_UTIL_METATOOLS_HH

#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_virtual_base_of.hpp>
#include <iostream>
#include <utility>
namespace karabo {
    namespace util {

        class Hash;

        template <class T>
        struct is_shared_ptr : boost::false_type {};

        template <class T>
        struct is_shared_ptr<boost::shared_ptr<T> > : boost::true_type {};

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
            // below is the specialisation for is_hash_base = boost::false_type.

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
            static const boost::shared_ptr<T> cast(const boost::shared_ptr<T>& v) {
                // if the compiler ever reaches this point compilation is to fail on purpose, as
                // we only support explicit setting of Hash::Pointer to the Hash
                inserting_derived_hash_classes_as_pointers_is_not_supported();
                return v;
            }


            void inserting_derived_hash_classes_as_pointers_is_not_supported();
        };


        template <>
        struct conditional_hash_cast<boost::false_type> {
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
            static boost::shared_ptr<T> cast(T* p) {
                return boost::static_pointer_cast<T>(p->shared_from_this());
            }
        };

        // if it is a virtual base a dynamic cast must be made

        template <>
        struct static_or_dyn_cast<boost::true_type, boost::true_type> {
            template <typename T>
            static boost::shared_ptr<T> cast(T* p) {
                return boost::dynamic_pointer_cast<T>(p->shared_from_this());
            }
        };


        // if this is not a direct base a dynamic cast must be made

        template <>
        struct static_or_dyn_cast<boost::false_type, boost::false_type> {
            template <typename T>
            static boost::shared_ptr<T> cast(T* p) {
                return boost::dynamic_pointer_cast<T>(p->shared_from_this());
            }
        };

        template <typename>
        struct cond_dyn_cast {
            template <typename T>
            static boost::shared_ptr<T> cast(T* p) {
                return p->shared_from_this();
            }
        };

        template <>
        struct cond_dyn_cast<std::false_type> {
            template <typename T>
            static boost::shared_ptr<T> cast(T* p) {
                typedef typename boost::is_virtual_base_of<decltype(*(p->shared_from_this())), T>::type is_virtual_base;
                typedef typename boost::is_base_of<decltype(*(p->shared_from_this())), T>::type is_base;
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
            typedef typename std::is_same<boost::shared_ptr<Obj>, decltype(o->shared_from_this())>::type is_same_type;
            boost::weak_ptr<const Obj> wp(cond_dyn_cast<is_same_type>::template cast(o));
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
         * destruction is not blocked if f is not being executed but only bound. Class Obj needs to derive from
         * boost::enable_shared_from_this and the object pointer o has to be hold by a shared_ptr. This means that you
         * cannot use bind_weak within the constructor of Obj nor for objects constructed on the stack. Note that f may
         * have any return type, but the bound functor will return void. Below is an example of how to bind to a
         * boost::asio interface.
         *
         * void Device::executeStepFunction(int arg, const boost::system::error_code& error) {
         *     ....
         *     m_timer.async_wait(bind_weak(&Device::executeStepFunction, this, arg + 1, _1));
         *     ....
         * }
         *
         * @param f: function to bind, give as &Foo::bar
         * @param o: pointer to object to bind to
         * @param p: parameters as one would give to boost::bind. Placeholders are fully supported.
         * @return: bound functor, compatible with boost bind.
         */
        template <typename F, typename Obj, typename... P>
        auto bind_weak(const F& f, Obj* const o, const P... p) -> decltype(boost::bind(exec_weak_impl(f, o), p...)) {
            // note that boost::arg<N>s cannot be forwarded, thus we work with references here.
            auto wrapped = exec_weak_impl(f, o);
            return boost::bind(wrapped, p...);
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
