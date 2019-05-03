#pragma once

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <unordered_map>

#include "type_info.h"
#include "context.h"

namespace uc
{
    namespace lip
    {
        class type_factory
        {
            public:

                void* placement_new( const load_context& c )
                {
                    return do_placement_new(c);
                }

            protected:
                virtual void* do_placement_new(const load_context& c) = 0;
        };

        template <typename t> inline t* placement_new(type_factory* f, const load_context& c )
        {
            return reinterpret_cast<t*>(f->placement_new(c));
        }

        template <bool = true> struct is_pod_selector {};
        template <> struct is_pod_selector<false> {};

        template <typename t> class typed_type_factory final : public type_factory
        {
            protected:

            void* do_placement_new(const load_context& c, is_pod_selector<true>)
            {
                return new (c.base_pointer()) t();
            }

            void* do_placement_new(const load_context& c, is_pod_selector<false> )
            {
                return new (c.base_pointer()) t(c);
            }

            void* do_placement_new(const load_context& c) override
            {
                return do_placement_new(c, is_pod_selector< std::is_pod<t>::value>());
            }


        };

        class type_factory_manager
        {
            public:

            void register_factory(type_identifier id, type_factory* f)
            {
                m_factories.insert(std::make_pair(id, f));
            }

            type_factory* get(type_identifier id) const
            {
                auto f = m_factories.find(id);

                if (f != m_factories.end())
                {
                    return f->second;
                }
                else
                {
                    return nullptr;
                }
            }

            private:

            std::unordered_map< type_identifier, type_factory* > m_factories;
        };

        inline type_factory_manager* type_factories()
        {
            static type_factory_manager m;
            return &m;
        }

        template <typename t> struct type_factory_registrar
        {
            type_factory_registrar()
            {
                static typed_type_factory<t> s_factory;
                type_factories()->register_factory( typename type_id<t>::value, &s_factory );
            }
        };
    }

}

