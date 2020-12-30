#pragma once

#include <utility>

namespace uc
{
    namespace util
    {
        namespace details
        {
            template<typename t>
            pimpl<t>::pimpl() : m{ new t{} } { }

            template<typename t>
            template<typename ...Args>
            pimpl<t>::pimpl(Args&& ...args)
                : m{ new t{ std::forward<Args>(args)... } } { }

            template<typename t>
            pimpl<t>::~pimpl() { }

            template<typename t>
            const t* pimpl<t>::operator->() const { return m.get(); }

            template<typename t>
            t* pimpl<t>::operator->() { return m.get(); }

            template<typename t>
            t& pimpl<t>::operator*() { return *m.get(); }

            template<typename t>
            const t& pimpl<t>::operator*() const { return *m.get(); }

            template<typename t>
            t* pimpl<t>::get() { return m.get(); }

            template<typename t>
            const t* pimpl<t>::get() const { return m.get(); }

            template<typename t>
            pimpl<t>::pimpl(pimpl&&) = default;

            template<typename t>
            pimpl<t>& pimpl<t>::operator=(pimpl&&) = default;
        }
    }
}


