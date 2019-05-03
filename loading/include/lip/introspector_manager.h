#pragma once

#include <cstdint>
#include <unordered_map>

#include "introspector.h"

namespace uc
{
    namespace lip
    {
        class introspector_manager
        {

            public:

            void register_introspector(type_identifier type, introspector_base* base)
            {
                m_introspectors.insert(std::make_pair(type, base));
            }

            template <typename t> 
            void register_introspector(introspector_base* base)
            {
                register_introspector(make_type_info<t>().type_id(), base);
            }

            introspector_base* get_introspector(type_identifier type) const
            {
                auto r = m_introspectors.find(type);

                if (r != std::end(m_introspectors))
                {
                    return r->second;
                }
                else
                {
                    return nullptr;
                }
            }


            private:

            std::unordered_map< type_identifier, introspector_base*> m_introspectors;
            
        };

        inline introspector_manager* introspector_database()
        {
            static introspector_manager m;
            return &m;
        }

        template <typename t> inline introspector_base* get_introspector()
        {
            return introspector_database()->get_introspector(make_type_info<t>().type_id());
        }

        template <typename t> inline introspector_base* get_introspector(const t*)
        {
            return introspector_database()->get_introspector(make_type_info<t>().type_id());
        }

        template <typename t> inline introspector_base* get_introspector(const t&)
        {
            return introspector_database()->get_introspector(make_type_info<t>().type_id());
        }

        inline introspector_base* get_introspector(type_identifier id)
        {
            return introspector_database()->get_introspector(id);
        }

    }
}



