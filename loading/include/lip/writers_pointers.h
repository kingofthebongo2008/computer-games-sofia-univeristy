#pragma once

#include <memory>

#include "member_info_base.h"
#include "pointers.h"
#include "tools_time_writer.h"

namespace uc
{
    namespace lip
    {
        template <typename t>  inline const reloc_pointer<t>* get_reloc_pointer(const void* root_object, size_t offset)
        {
            uintptr_t ptr  = (reinterpret_cast<uintptr_t>(root_object) + offset);
            return reinterpret_cast< const reloc_pointer<t> * >( ptr );
        }

        template <typename t> struct write_smart_object< reloc_pointer< t > >
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                void write_object(const void* root_object, const uc::lip::introspector_base* is, tools_time_writer& w);

                auto w = reinterpret_cast<tools_time_writer*>(ctx);

                auto is     = lip::get_introspector< t >();
                auto root   = get_reloc_pointer<t>(root_object, offset);

                if (root->get())
                {
                        w->write_pointer_location(offset);
                        //start new root and write it, if not written
                        write_object(root->get(), is, *w);
                }
            }
        };
    }

}
