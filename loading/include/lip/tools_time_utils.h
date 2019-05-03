#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "tools_time_writer.h"


namespace uc
{
    namespace lip
    {
        template <typename compressor>
        inline void write_compressed_data(const std::vector<uint8_t>& buffer, const std::string& file_name, const compressor& c)
        {
            auto compressed_data = c(buffer);
            std::fstream f(file_name, std::ios_base::out | std::ios_base::binary); //todo: disable file caching
                                                                                   //write header 8 bytes
            f << "LZHAM   ";
            //write decompressed size
            uint64_t size = buffer.size();
            f.write((const char*)&size, sizeof(size));
            f.write((const char*)&compressed_data[0], compressed_data.size());
        }

        template <typename compressor>
        inline void write_compressed_data(const std::vector<uint8_t>& buffer, const std::wstring& file_name, const compressor& c)
        {
            auto compressed_data = c(buffer);
            std::fstream f(file_name, std::ios_base::out | std::ios_base::binary); //todo: disable file caching
                                                                                   //write header 8 bytes
            f << "LZHAM   ";
            //write decompressed size
            uint64_t size = buffer.size();
            f.write((const char*)&size, sizeof(size));
            f.write((const char*)&compressed_data[0], compressed_data.size());
        }

        /*
        inline void write_compressed_data(std::vector<uint8_t>&& buffer, const std::string& file_name)
        {
            auto compressed_data = lzham::compress_buffer(std::move(buffer));
            std::fstream f(file_name, std::ios_base::out | std::ios_base::binary); //todo: disable file caching
                                                                                   //write header 8 bytes
            f << "LZHAM   ";
            //write decompressed size
            uint64_t size = buffer.size();
            f.write((const char*)&size, sizeof(size));
            f.write((const char*)&compressed_data[0], compressed_data.size());
        }
        */

        template <typename lip_type> std::vector<uint8_t> inline binarize_object( const lip_type* o )
        {
            lip::tools_time_writer w;
            auto is = lip::get_introspector<lip_type>();
            lip::write_object(o, is, w);

            //write these bytes in a file
            return w.finalize();
        }

        template <typename lip_type> std::vector<uint8_t> inline binarize_object(std::unique_ptr<lip_type>&& o)
        {
            lip::tools_time_writer w;
            auto is = lip::get_introspector<lip_type>();
            lip::write_object(o.get(), is, w);
            //write these bytes in a file
            auto r = w.finalize();
            return r;
        }

        template <typename lip_type, typename compressor > inline void serialize_object( const lip_type* o, const std::string& file_name, const compressor& c )
        {
            write_compressed_data( binarize_object( o ), file_name, c);
        }

        template <typename lip_type, typename compressor > inline void serialize_object(const lip_type* o, const std::wstring& file_name, const compressor& c)
        {
            write_compressed_data(binarize_object(o), file_name, c);
        }

        template <typename lip_type, typename compressor > inline void serialize_object( std::unique_ptr<lip_type>&& o, const std::string& file_name, const compressor& c)
        {
            write_compressed_data( binarize_object(std::move(o) ), file_name, c);
        }

        template <typename lip_type, typename compressor> inline void serialize_object(std::unique_ptr<lip_type>&& o, const std::wstring& file_name, const compressor& c)
        {
            write_compressed_data(binarize_object(std::move(o)), file_name, c);
        }
    }
}



