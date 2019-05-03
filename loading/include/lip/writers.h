#pragma once

#include <memory>

#include "member_info_base.h"
#include "pointers.h"
#include "tools_time_writer.h"

namespace uc
{
    namespace lip
    {

        template <> struct write_smart_object<char>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(char));
            }
        };

        template <> struct write_smart_object<unsigned char>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(unsigned char));
            }
        };

        template <> struct write_smart_object<short>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(short));
            }
        };

        template <> struct write_smart_object<unsigned short>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(unsigned short));
            }
        };

        template <> struct write_smart_object<int>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(int));
            }
        };

        template <> struct write_smart_object<unsigned int>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(float));
            }
        };

        template <> struct write_smart_object<long>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(long));
            }
        };

        template <> struct write_smart_object<unsigned long>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(unsigned long));
            }
        };

        template <> struct write_smart_object<long long>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(long long));
            }
        };

        template <> struct write_smart_object<unsigned long long>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(unsigned long long));
            }
        };

        template <> struct write_smart_object<float>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(float));
            }
        };

        template <> struct write_smart_object<double>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(double));
            }
        };

        template <> struct write_smart_object<bool>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(bool));
            }
        };

        template <> struct write_smart_object<signed char>
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                auto w = reinterpret_cast<tools_time_writer*>(ctx);
                w->copy(offset, sizeof(signed char));
            }
        };
    }

}
