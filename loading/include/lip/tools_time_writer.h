#pragma once

#include <assert.h>
#include <cstdint>
#include <stack>
#include <vector>
#include <unordered_map>

#include "introspector.h"

namespace uc
{
    namespace lip
    {
        struct struct_contents
        {
            enum class pointer_type : uint8_t
            {
                polymorphic,
                reloc,
                weak,
            };

            struct_contents(uintptr_t address, size_t size, uint32_t alignment) :
                m_start_address(address)
                , m_size(size)
                , m_alignment(alignment)
            {
                m_data.resize(size);
            }

            uintptr_t                   m_start_address;
            size_t                      m_size;
            uint32_t                    m_alignment;

            std::vector<uint8_t>        m_data;

            std::vector<uintptr_t>      m_pointer_locations;        //all types of pointers
            std::vector<pointer_type>   m_pointer_locations_types;  //types of pointers
        };

        namespace details
        {
			#if defined(_X64)
            static void patch_pointer(std::vector<uint8_t> &stream, uint64_t d, uint64_t index)
            {
                //todo: push_back is very expensive for a byte
                stream[index + 0] = ((uint8_t)((d & 0xff)));
                stream[index + 1] = ((uint8_t)((d & 0xff00) >> 8));
                stream[index + 2] = ((uint8_t)((d & 0xff0000) >> 16));
                stream[index + 3] = ((uint8_t)((d & 0xff000000) >> 24));
                stream[index + 4] = ((uint8_t)((d & 0xff00000000) >> 32));
                stream[index + 5] = ((uint8_t)((d & 0xff0000000000) >> 40));
                stream[index + 6] = ((uint8_t)((d & 0xff000000000000) >> 48));
                stream[index + 7] = ((uint8_t)((d & 0xff00000000000000) >> 56));
            }
			#else
			static void patch_pointer(std::vector<uint8_t> &stream, uint32_t d, uint32_t index)
			{
				//todo: push_back is very expensive for a byte
				stream[index + 0] = ((uint8_t)((d & 0xff)));
				stream[index + 1] = ((uint8_t)((d & 0xff00) >> 8));
				stream[index + 2] = ((uint8_t)((d & 0xff0000) >> 16));
				stream[index + 3] = ((uint8_t)((d & 0xff000000) >> 24));

				/*
				stream[index + 4] = ((uint8_t)((d & 0xff00000000) >> 32));
				stream[index + 5] = ((uint8_t)((d & 0xff0000000000) >> 40));
				stream[index + 6] = ((uint8_t)((d & 0xff000000000000) >> 48));
				stream[index + 7] = ((uint8_t)((d & 0xff00000000000000) >> 56));
				*/
			}
			#endif


        }

        //state machine which records pointers and writes structs into an array
        struct tools_time_writer
        {
            std::stack<size_t>              m_current_structs;   // this holds a stack of indices into the struct_info ector, for quick access.
            std::vector<struct_contents>    m_struct_info;       // held and written in the order supplied by the user


            void begin_struct( const void *s, size_t size, uint32_t alignment )
            {
                struct_contents sc(reinterpret_cast<uintptr_t>(s), size, alignment);

                m_current_structs.push(m_struct_info.size());     // put index onto the top of the stack
                m_struct_info.push_back(sc);                        // add new struct
            }

            void end_struct()
            {
                assert(m_current_structs.size());
                m_current_structs.pop();
            }

            void copy( size_t offset, size_t bytes )
            {
                struct_contents &sc = m_struct_info[m_current_structs.top()];
                std::memcpy(&sc.m_data[offset], reinterpret_cast<uint8_t*> (sc.m_start_address + offset), bytes);
            }

            void write_pointer(size_t offset, uintptr_t value)
            {
                struct_contents &sc = m_struct_info[m_current_structs.top()];
                std::memcpy(&sc.m_data[offset], &value, sizeof(value));
            }

            void write_pointer_location( size_t offset )
            {
                struct_contents &sc = m_struct_info[m_current_structs.top()];

                //sizeof unique_ptr is 16 bytes, due to custom deleters
                uintptr_t ptr               = ( sc.m_start_address + offset );
                uintptr_t pointer_offset    = ptr + sizeof(uintptr_t);

                //copy zero here
                std::memset( &sc.m_data[ offset ], 0, 2 * sizeof(uintptr_t) );

                // remember this is where a pointer is
                sc.m_pointer_locations.push_back( pointer_offset );
                sc.m_pointer_locations_types.push_back( struct_contents::pointer_type::reloc );
            }

            void write_raw_pointer_location(size_t offset)
            {
                struct_contents &sc = m_struct_info[m_current_structs.top()];

                //sizeof unique_ptr is 16 bytes, due to custom deleters
                uintptr_t ptr = (sc.m_start_address + offset);
                uintptr_t pointer_offset = ptr;

                //copy zero here
                std::memset(&sc.m_data[offset], 0, 1 * sizeof(uintptr_t));

                // remember this is where a pointer is
                sc.m_pointer_locations.push_back(pointer_offset);
                sc.m_pointer_locations_types.push_back(struct_contents::pointer_type::reloc);
            }

            bool object_is_written(const void* root_object) const
            {
                return lookup_pointer_target(reinterpret_cast<uintptr_t>(root_object)) != nullptr;
            }

            //-------------------
            const struct_contents *lookup_pointer_target( uintptr_t ptr ) const
            {
                // slow walk through all structs looking for one where pointer targets the specified range
                for (auto i = 0U; i < m_struct_info.size(); i++)
                {
                    const struct_contents &sc = m_struct_info[i];
                    if (ptr >= sc.m_start_address && ptr < sc.m_start_address + static_cast<intptr_t> (sc.m_data.size()))
                        return &sc;
                }

                return nullptr;
            }

            inline size_t align(size_t s, size_t alignment)
            {
                //return ( (s + alignment - 1) / alignment ) * alignment;
                return s + (alignment - 1)  & ~(alignment - 1);
            }

            std::vector<uint8_t> finalize()
            {
                assert(m_current_structs.size() == 0);

                std::vector<uint8_t> block;

                // while building the final data set, we remember where each structure ended up, so we can make relative fixups to all the pointers
                std::unordered_map<uintptr_t, uintptr_t> address_to_index;


                std::vector< struct_contents* > ptrs;
                

                for (auto i = 0U; i < m_struct_info.size(); ++i)
                {
                    ptrs.push_back(&m_struct_info[i]);
                }

                auto contents = m_struct_info[0].m_start_address;
         
                //sort by start address, to make array elements continous
                std::sort(ptrs.begin(), ptrs.end(), [contents]( const auto a, const auto b )
                {
                    auto addr_a = a->m_start_address;
                    auto addr_b = b->m_start_address;
                    
                    return (addr_a < addr_b);
                });

                //split and copy then and make the original root object start of the structs
                std::vector< struct_contents* > less;
                std::vector< struct_contents* > greater;

                for ( auto&& s : ptrs )
                {
                    if (s->m_start_address < contents)
                    {
                        less.push_back(s);
                    }
                    else
                    {
                        greater.push_back(s);
                    }
                }

                std::copy(greater.begin(), greater.end(), ptrs.begin());
                std::copy(less.begin(), less.end(), ptrs.begin() + greater.size());

                size_t reserve_size = 0;
                // Walk all the structs and reserve memory
                for (auto i = 0U; i < ptrs.size(); i++)
                {
                    struct_contents* sc = ptrs[i];
                    auto aligned_size   = align(sc->m_data.size(), sc->m_alignment);
                    reserve_size += aligned_size;
                }
                block.reserve(reserve_size);

                // Walk all the structs and concatenate all the structs into a single block.
                for (auto i = 0U; i < ptrs.size(); i++)
                {
                    struct_contents* sc = ptrs[i];

                    auto aligned_size = align( block.size(), sc->m_alignment ); 

                    address_to_index[ sc->m_start_address ] = aligned_size;    //remember where this struct starts

                    auto old_size = aligned_size;
                    auto new_size = align( old_size + sc->m_data.size(), sc->m_alignment );
                    
                    block.resize( new_size );
                    std::memcpy(&block[old_size], &sc->m_data[0], sc->m_data.size());
                }

                // finally, fixup all the pointers
                for (auto i = 0U; i < ptrs.size(); i++)
                {
                    struct_contents* sc = ptrs[i];

                    const auto start_of_struct = address_to_index[sc->m_start_address];

                    for (auto&& ptr  : sc->m_pointer_locations )
                    {
                        // determine where the pointer is stored in the data block
                        const auto index_to_pointer_storage = start_of_struct + (ptr - sc->m_start_address);

                        // find out the index of where the pointer points TO in the data block.  If LookupPointerTarget fails, it's because
                        // the pointer points to some structure that was not written out, or not entirely written out.
                        uintptr_t const address_pointed_to = ptr ? *( uintptr_t * )ptr : 0;

                        auto    target = lookup_pointer_target( address_pointed_to );

                        //assert( target != nullptr ); //missing serialization pointer

                        const auto index_to_target = target ? address_to_index[ target->m_start_address ] + address_pointed_to - target->m_start_address : index_to_pointer_storage;  // null points to itself

						
						//negative offsets are valid
						const auto relative_offset				= (intptr_t) index_to_target - (intptr_t) start_of_struct;
						const uintptr_t* relative_offset_ptr	= reinterpret_cast< const uintptr_t* >(&relative_offset);
						details::patch_pointer(block, *relative_offset_ptr, index_to_pointer_storage);
                    }
                }

                return block;
            }
        };

        inline const void* object_address(const void* root_object, size_t root_offset)
        {
            uintptr_t r = reinterpret_cast<uintptr_t>(root_object);
            return reinterpret_cast<const void*> (r + root_offset);
        }

        inline void write_smart_member_object( const void* root_object, size_t offset, const uc::lip::introspector_base* , const uc::lip::member_info_base* m, tools_time_writer& w)
        {
            m->write_object(root_object, offset, &w);
        }

        inline void write_object( const void* root_object, size_t root_offset, bool write_type_id, const uc::lip::introspector_base* is, tools_time_writer& w)
        {
            if (is->is_polymorphic() && write_type_id)
            {
                w.write_pointer(root_offset, is->type_id());
            }

            //write base classes
            is->for_each_base_class([root_object, root_offset, &w](const auto& bi)
            {
                auto base_class_is = uc::lip::get_introspector(bi->type_id());
                write_object(root_object, root_offset + bi->offset(), false, base_class_is, w);
            });

            //write members
            is->for_each_member([root_object, root_offset, &w](const auto& mi)
            {
                auto member_is = uc::lip::get_introspector(mi->type_id());

                if (mi->is_array())
                {
                    auto s = mi->array_size();

                    if (mi->is_smart_member())
                    {
                        for (auto i = 0U; i < s; ++i)
                        {
                            write_smart_member_object( root_object, root_offset + mi->offset() + i * member_is->size(), member_is, mi.get(), w );
                        }
                    }
                    else
                    {
                        for (auto i = 0U; i < s; ++i)
                        {
                            write_object(root_object, root_offset + mi->offset() + i * member_is->size(), true, member_is, w);
                        }
                    }
                }
                else
                {
                    if (mi->is_smart_member())
                    {
                        write_smart_member_object(root_object, root_offset + mi->offset(), member_is, mi.get(), w);
                    }
                    else
                    {
                        write_object(root_object, root_offset + mi->offset(), true, member_is, w);
                    }
                }
            });
        }

        inline void write_object( const void* root_object, const uc::lip::introspector_base* is, tools_time_writer& w )
        {
            if ( !w.object_is_written( root_object) )
            {
                w.begin_struct(object_address(root_object, 0), is->size(), is->alignment());
                write_object(root_object, 0, true, is, w);
                w.end_struct();
            }
        }
    }
}



