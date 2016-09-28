/*
 * File:   MetaTools.hh
 * Author: heisenb
 *
 * Created on August 10, 2016, 3:56 PM
 */

#ifndef KARABO_UTIL_METATOOLS_HH
#define	KARABO_UTIL_METATOOLS_HH

#include <boost/type_traits/is_virtual_base_of.hpp>

namespace karabo {
    namespace util {

        class Hash;

        template <class T>
        struct is_shared_ptr : boost::false_type {

        };

        template <class T>
        struct is_shared_ptr<boost::shared_ptr<T> > : boost::true_type {

        };

        template<typename is_hash_base>
        struct conditional_hash_cast {

            template<typename T>
            static const Hash& cast(const T& v) {
                return reinterpret_cast<const Hash&> (v);
            }

            template<typename T>
            static const boost::shared_ptr<T> cast(const boost::shared_ptr<T>& v) {
                //if the compiler ever reaches this point compilation is to fail on purpose, as
                //we only support explicit setting of Hash::Pointer to the Hash
                inserting_derived_hash_classes_as_pointers_is_not_supported();
                return v;
            }


            void inserting_derived_hash_classes_as_pointers_is_not_supported();
        };

        template<>
        struct conditional_hash_cast<boost::false_type> {

            template<typename T>
            static T& cast(T& v) {
                return v;
            }

            template<typename T>
            static const T& cast(const T& v) {
                return v;
            }



        };

        /**
         * Helper functions to compile-time distinguish if a dynamic_cast is needed.
         */

        //if it is not a virtual base but is a direct base we can static cast

        template<typename, typename>
        struct static_or_dyn_cast {

            template<typename T>
            static boost::shared_ptr<T> cast(T* p) {
                return boost::static_pointer_cast<T>(p->shared_from_this());
            }
        };

        //if it is a virtual base a dynamic cast must be made

        template<>
        struct static_or_dyn_cast<boost::true_type, boost::true_type> {

            template<typename T>
            static boost::shared_ptr<T> cast(T* p) {
                return boost::dynamic_pointer_cast<T>(p->shared_from_this());
            }
        };


        // if this is not a direct base a dynamic cast must be made

        template<>
        struct static_or_dyn_cast<boost::false_type, boost::false_type> {

            template<typename T>
            static boost::shared_ptr<T> cast(T* p) {
                return boost::dynamic_pointer_cast<T>(p->shared_from_this());
            }
        };

        template<typename>
        struct cond_dyn_cast {

            template<typename T>
            static boost::shared_ptr<T> cast(T* p) {
                return p->shared_from_this();
            }
        };

        template<>
        struct cond_dyn_cast<std::false_type> {

            template<typename T>
            static boost::shared_ptr<T> cast(T* p) {
                typedef typename boost::is_virtual_base_of < decltype(*(p->shared_from_this())), T>::type is_virtual_base;
                typedef typename boost::is_base_of < decltype(*(p->shared_from_this())), T>::type is_base;
                return static_or_dyn_cast<is_virtual_base, is_base>::cast(p);
            }
        };

        /**
         * Provides a wrapper with the same signature as f, but securing shared ownership of an object O before execution of f.
         * Class O needs to derive somewhere in its inheritance tree from enable_shared_from_this();
         * @param f: an arbitrary member function, can have any argument types but must return void
         * @return a wrapped version of f.
         */
        template<typename Ret, typename... Args, typename Obj>
        std::function<void(Args&...) > exec_weak_impl(Ret(Obj::*f)(Args...), Obj* o) {
            typedef typename std::is_same < boost::shared_ptr<Obj>, decltype(o->shared_from_this())>::type is_same_type;
            boost::weak_ptr<Obj> wp(cond_dyn_cast<is_same_type>::template cast(o));
            //we need to copy-capture here -> otherwise segfault, because f and wp go out of scope
            auto wrapped = [f, wp](Args&... fargs) {

                auto ptr = wp.lock();
                if (ptr) {
                    (*ptr.*f)(fargs...);
                }

            };
            return wrapped;
        }


        /**
         * Weakly binds function f to an object of class O, but assures shared ownership of the object while f is executed.
         * This means that during the lifetime of calling f, the object cannot be destroyed, but destruction is not blocked
         * if f is not being executed but only bound. Below is an example of how to bind to a boost::asio interface.
         *
         * void Device::executeStepFunction(int arg, const boost::system::error_code& error) {
         *     ....
         *     m_timer.async_wait(bind_weak(&Device::executeStepFunction, this, arg + 1, _1));
         *     ....
         * }
         *
         * @param f: function to bind, give as &Foo::bar
         * @param o: object to bind to, needs to derive from boost::enable_shared_from_this
         * @param p: parameters as one would give to boost::bind. Placeholders are fully supported.
         * @return: bound functor, compatible with boost bind.
         */
        template< typename F, typename Obj, typename ...P>
        auto bind_weak(const F& f, Obj * const o, const P&... p) -> decltype(boost::bind<void>(exec_weak_impl(f, o), p...)) {
            //note that boost::arg<N>s cannot be forwarded, thus we work with references here.
            auto wrapped = exec_weak_impl(f, o);
            return boost::bind<void>(wrapped, p...);
        }



    }
}


#endif	/* METATOOLS_HH */

