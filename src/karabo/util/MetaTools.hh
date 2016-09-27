/*
 * File:   MetaTools.hh
 * Author: heisenb
 *
 * Created on August 10, 2016, 3:56 PM
 */

#ifndef KARABO_UTIL_METATOOLS_HH
#define	KARABO_UTIL_METATOOLS_HH

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
        template<typename>
        struct cond_dyn_cast {

            template<typename T>
            static boost::shared_ptr<T>& cast(T* p) {
                return p->shared_from_this();
            }
        };

        template<>
        struct cond_dyn_cast<std::false_type> {

            template<typename T>
            static boost::shared_ptr<T> cast(T* p) {
                return boost::dynamic_pointer_cast<T>(p->shared_from_this());
            }
        };

        /**
         * Provides a wrapper with the same signature as f, but securing shared ownership of an object O before execution of f.
         * Class O needs to derive somewhere in its inheritance tree from enable_shared_from_this();
         * @param f: an arbitrary member function, can have any argument types but must return void
         * @return a wrapped version of f.
         */
        template<typename Ret, typename... Args, typename O>
        std::function<void(O*, Args&...) > exec_weak_impl(Ret(O::*f)(Args...)) {
            auto wrapped = [f](O* o, Args&... fargs) { //we need to copy-capture here -> otherwise segfault, because f out of scope
                auto wp = boost::weak_ptr<O>(cond_dyn_cast<typename std::is_same < boost::shared_ptr<O>, decltype(o->shared_from_this())>::type>::template cast(o));
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
        template< typename F, typename O, typename ...P>
        auto bind_weak(const F& f, O * const o, const P&... p) -> decltype(boost::bind<void>(exec_weak_impl(f), o, p...)) {
            //note that boost::arg<N>s cannot be forwarded, thus we work with references here.
            auto wrapped = exec_weak_impl(f);
            return boost::bind<void>(wrapped, o, p...);
        }



    }
}


#endif	/* METATOOLS_HH */

