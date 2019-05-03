#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>


#include "member_info.h"
#include "base_class_info.h"
 

namespace uc
{
    namespace lip
    {
        inline bool operator < (const std::unique_ptr<base_class_info_base>& a, const std::unique_ptr<base_class_info_base>& b)
        {
            return a->offset() < b->offset();
        }

        inline bool operator < (const std::unique_ptr<member_info_base>& a, const std::unique_ptr<base_class_info_base>& b)
        {
            return a->offset() < b->offset();
        }

        class introspector_base 
        {
            private:

            std::vector< std::unique_ptr<base_class_info_base >  > m_base_classes;
            std::vector< std::unique_ptr<member_info_base> >       m_members;

            type_info   m_type_info;

            public:

            introspector_base(const type_info& ti) : m_type_info(ti)
            {

            }

            template <typename t, typename member_type_t>
            void register_member( const char* member_name, member_type_t(t::*member_ptr) )
            {
                m_members.emplace_back(std::unique_ptr< member_info<t, member_type_t> >(make_member_info_ptr(member_name, member_ptr)));
            }

            template <typename t, typename member_type_t, size_t size>
            void register_member(const char* member_name, member_type_t(t::*member_ptr)[size])
            {
                m_members.emplace_back(std::unique_ptr< array_member_info<t, member_type_t, size >>(make_array_member_info_ptr(member_name, member_ptr)));
            }

            template <typename t, typename base_class_type_t>
            void register_base_class()
            {
                m_base_classes.emplace_back(std::make_unique < base_class_info< t, base_class_type_t> >());
            }

            void sort()
            {
                std::sort( std::begin(m_base_classes), std::end(m_base_classes));
                std::sort( std::begin(m_members), std::end(m_members));
            }

            size_t size() const
            {
                return m_type_info.size();
            }

            uint32_t alignment() const
            {
                return m_type_info.alignment();
            }

            type_identifier type_id() const
            {
                return m_type_info.type_id();
            }

            bool is_polymorphic() const
            {
                return m_type_info.is_polymorphic();
            }


            template <typename f>
            void for_each_base_class(const f& f) const
            {
                std::for_each(std::cbegin(m_base_classes), std::cend(m_base_classes), f);
            }

            template <typename f>
            void for_each_member(const f& f) const
            {
                std::for_each(std::cbegin(m_members), std::cend(m_members), f);
            }

            private:

            introspector_base(const introspector_base&) = delete;
            introspector_base& operator=(const introspector_base&) = delete;
        };

        template <typename t>
        class introspector : public introspector_base
        {
            using base = introspector_base;

            public:

            introspector() : base( make_type_info<t>())
            {

            }

            template <typename member_type_t>
            void register_member(const char* member_name, member_type_t(t::*member_ptr))
            {
                base::register_member<t, member_type_t>(member_name, member_ptr);
            }

            template <typename member_type_t, size_t size>
            void register_member(const char* member_name, member_type_t(t::*member_ptr)[size])
            {
                base::register_member<t, member_type_t, size>(member_name, member_ptr);
            }

            template <typename base_class_type_t>
            void register_base_class()
            {
                base::register_base_class<t, base_class_type_t>();
            }
        };
     
     

        template <>
        class introspector<char> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<char>())
            {

            }
        };

        template <>
        class introspector<unsigned char> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<unsigned char>())
            {

            }
        };

        template <>
        class introspector<short> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<short>())
            {

            }
        };

        template <>
        class introspector<unsigned short> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<unsigned short>())
            {

            }
        };

        template <>
        class introspector<int> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<int>())
            {

            }
        };

        template <>
        class introspector<unsigned int> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<unsigned int>())
            {

            }
        };

        template <>
        class introspector<unsigned long> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<unsigned long>())
            {

            }
        };

        template <>
        class introspector<long long> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<long long>())
            {

            }
        };




        template <>
        class introspector<unsigned long long> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<unsigned long long>())
            {

            }
        };

        template <>
        class introspector<float> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<float>())
            {

            }
        };

        template <>
        class introspector<double> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<double>())
            {

            }
        };

        template <>
        class introspector<bool> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<bool>())
            {

            }
        };

        template <>
        class introspector<signed char> : public introspector_base
        {
            using base = introspector_base;

        public:

            introspector() : base(make_type_info<signed char>())
            {

            }
        };


    }
}



