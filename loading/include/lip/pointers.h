#pragma once

#include <memory>
#include <iterator>
#include <algorithm>
#include <type_traits>

namespace uc
{
    namespace lip
    {
        template <typename t> struct deleter
        {
            deleter() = default;

            enum free_policy
            {
                in_place,
                do_free
            };

            explicit deleter(free_policy policy) : m_policy(policy)
            {

            }

            void operator()( t* object )
            {
                if (object)
                {
                    if (m_policy == in_place)
                    {
                        object->~t();
                    }
                    else
                    {
                        delete object;
                    }
                }
            }

            free_policy m_policy = do_free;
        };


        inline load_context make_pointer_load_context(const load_context& parent, intptr_t offset)
        {
            return load_context(reinterpret_cast< void* > (reinterpret_cast<intptr_t>(parent.base_pointer()) + offset));
        }

        inline load_context make_pointer_load_context(const load_context& parent, void* offset_pointer)
        {
            //we expect in offset_pointer to have an offset of the parent, written by the serialization system
            intptr_t offset = reinterpret_cast<intptr_t>(offset_pointer);

            if (offset > 0)
            {
                return make_pointer_load_context(parent, offset);
            }
            else
            {
                return load_context();
            }
        }

        inline load_context make_load_context ( void* pointer )
        {
            return load_context(pointer);
        }

        inline load_context make_load_context(uintptr_t pointer)
        {
            return load_context( reinterpret_cast<void*>(pointer) );
        }

        template <typename t> inline t* placement_new(const load_context& c, bool is_fundamental)
        {
            void* base_pointer = c.base_pointer();

            if (base_pointer)
            {
                return reinterpret_cast<t*>(base_pointer);
            }
            else
            {
                return nullptr;
            }
        }

        //todo: make this
        template <typename t> struct has_load_context_constructor : std::false_type {};
        template <typename t> struct has_load_context_constructor ;

        template <typename t> inline t* placement_new_impl( const load_context& c, std::false_type )
        {
            void* base_pointer = c.base_pointer();

            if (base_pointer)
            {
                return new (base_pointer) t(c);
            }
            else
            {
                return nullptr;
            }
        }

        template <typename t> inline t* placement_new_impl(const load_context& c, std::true_type )
        {
            void* base_pointer = c.base_pointer();

            if (base_pointer)
            {
                return reinterpret_cast<t*>(base_pointer);// new (base_pointer) t();
            }
            else
            {
                return nullptr;
            }
        }

        template <typename t> inline t* placement_new(const load_context& c)
        {
            return placement_new_impl<t>(c, std::is_pod<t>());
        }

        template <typename t> class reloc_pointer : public std::unique_ptr< t, deleter<t> >
        {

            using base = std::unique_ptr<t, deleter<t> >;

            public:

            using base::base;

            reloc_pointer()
            {

            }

            reloc_pointer( const load_context& c ) : reloc_pointer( placement_new<t>( make_pointer_load_context( c, get() )))
            {
                get_deleter().m_policy = deleter<t>::free_policy::in_place;
            }
        };

        template <typename t>
        struct member_info_typed_base_traits < reloc_pointer<t> >
        {
            static const constexpr bool is_smart_member = true;
        };
        
        template <typename t> class reloc_polymorphic_pointer : public std::unique_ptr< t, deleter<t> >
        {
            using base = std::unique_ptr<t, deleter<t> >;
            using base::base;
        };

        template <typename t> class weak_reloc_pointer
        {
         
        };

        template <typename t, size_t s > class reloc_fixed_array
        {
            
        };

        template <typename t > class weak_reloc_array
        {

        };


        template <typename t> void destroy( t* begin, t* end, std::false_type )
        {
            for (auto i = begin; i != end; ++i)
            {
                i->~t();
            }
        }

        template <typename t> void destroy(t*, t* , std::true_type)
        {

        }

        template <typename t > class reloc_polymorphic_array
        {

        };

        template <typename t > class reloc_weak_pointer_array
        {

        };

        template <size_t alignment = 4 > class binary_block_pointer
        {

        };

        template < typename t >
        inline reloc_pointer< t >  make_unique( void* address )
        {
            load_context c(address);
            return reloc_pointer<t>(new (address) t(c), deleter<t>(deleter<t>::in_place));
        }

        template < typename t, typename ...args >
        inline reloc_pointer< t >  make_unique( args&&... a )
        {
            return reloc_pointer< t >(new t(std::forward<args>(a)...), deleter<t>(deleter<t>::do_free) );
        }

        template < typename t >
        inline reloc_polymorphic_pointer< t > make_polymorphic_unique( void* address, type_identifier type_id )
        {
            auto factory = type_factories()->get(static_cast<type_identifier>(type_id));
            load_context c(address);
            return reloc_polymorphic_pointer<t>(placement_new<t>(factory, c), deleter<t>(deleter<t>::in_place) );
        }

        template < typename t >
        inline reloc_polymorphic_pointer<t> make_polymorphic_unique(void* address)
        {
            //assume the type id is stored in the first 8 bytes of the address
            uintptr_t type_id = *(reinterpret_cast<uintptr_t*>(address));
            return make_polymorphic_unique < t >(address, static_cast<type_identifier>(type_id));
        }
    }

}
