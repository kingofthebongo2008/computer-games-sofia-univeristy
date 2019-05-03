#pragma once

#include <memory>
#include <iterator>
#include <algorithm>

#include "pointers.h"

namespace uc
{
    namespace lip
    {
        //tools time vector with reallocation
        template <typename t > class reloc_data
        {
            public:

            using size_type              = size_t;
            using value_type             = t;
            using this_type              = reloc_data<t>;
            using iterator               = t*;
            using const_iterator         = const t*;

            using reverse_iterator       = t*;
            using const_reverse_iterator = const t*;
            using reference              = t&;
            using const_reference        = const t&;

            public:

            reloc_data() : m_data(nullptr), m_size(0), m_capacity(0)
            {

            }

            reloc_data( const load_context& c )
            { 
                //make the offset to the real data
                uintptr_t pointer = reinterpret_cast<intptr_t>(c.base_pointer()) + reinterpret_cast<intptr_t>(this->m_data);
                m_data = reinterpret_cast<t*>(pointer);

                if (is_pod<t>::value)
                {
                    //do nothing
                }
                else
                {
                    //m_size, m_capacity are already setup up
                    for (auto i = 0U; i < m_size; ++i)
                    {
                        auto element = m_data + i;
                        element = placement_new<t>(make_load_context(element));
                    }
                }
            }
            
            ~reloc_data() noexcept
            {
                destroy(begin(), end(), std::is_trivially_destructible<t>() );
            }

            size_type capacity() const noexcept
            {
                return m_capacity;
            }

            iterator begin() noexcept
            {
                return m_data;
            }

            const_iterator begin() const noexcept
            {
                return m_data;
            }

            iterator end() noexcept
            {
                return m_data + m_size;
            }

            const_iterator end() const noexcept
            {
                return m_data + m_size;
            }

            const_iterator cbegin() const noexcept
            {
                return m_data;
            }

            const_iterator cend() const noexcept
            {
                return m_data + m_size;
            }

            size_type size() const noexcept
            {
                return m_size;
            }

            bool empty() const noexcept
            {
                return begin() == end();
            }

            const_reference at(size_type position) const
            {
                return *(begin() + position);
            }

            reference at(size_type position)
            {
                return *(begin() + position);
            }

            const_reference operator[](size_type position) const
            {
                return *(begin() + position);
            }

            reference operator[](size_type position)
            {
                return *(begin() + position);
            }

            value_type * data() noexcept
            {
                return m_data;
            }

            const value_type * data() const noexcept
            {
                return m_data;
            }

            reference front()
            {
                return *m_data;
            }

            const_reference front() const
            {
                return *m_data;
            }

            reference back()
            {
                return *(end() - 1);
            }

            const_reference back() const
            {
                return *(end() - 1);
            }

            protected:
            t*               m_data;
            size_t           m_size;
            size_t           m_capacity;
            bool             m_enable_allocaton = false;
        };

        template <typename t > class reloc_vector
        {

        private:

            using size_type = size_t;
            using value_type = t;
            using this_type = reloc_vector<t>;
            using iterator = t*;
            using const_iterator = const t*;

            using reverse_iterator = t*;
            using const_reverse_iterator = const t*;

            using reference = t&;
            using const_reference = const t&;

            struct free_deleter
            {
                void operator()(void* p) const
                {
                    std::free(p);
                }
            };

            size_type grow_to(size_t new_capacity) const noexcept
            {
                // grow by 50% or at least to _Count
                size_type c = capacity();

                c = max_size() - c / 2 < c ? 0 : c + c / 2;	// try to grow by 50%

                if (c < new_capacity)
                    c = new_capacity;
                return c;
            }

            [[noreturn]] void _Xlen() const
            {
                //todo:: report errors
                // report a length_error
                //_Xlength_error("vector<T> too long");
            }

            [[noreturn]] void _Xran() const
            {
                //todo:: report errors
                // report an out_of_range error
                //_Xout_of_range("invalid vector<T> subscript");
            }

            std::unique_ptr< t, free_deleter> reallocate2(size_type count)
            {
                assert(m_enable_allocation);

                std::unique_ptr< t, free_deleter > p((t*)std::malloc(count * sizeof(t)), free_deleter());

                auto b = begin();
                auto e = end();
                auto d = p.get();

                auto range = e - b;

                for (auto&& i = b; i != e; i++, d++)
                {
                    *d = std::move(*i);
                }

                for (auto i = 0U; i < count - range; ++i)
                {
                    new ( d + i ) t();
                }
               
                return p;
            }

            void reallocate(size_type count)
            {
                m_data = reallocate2(count).release();
                m_capacity = count;
            }

            size_type unused_capacity() const
            {
                return m_capacity - m_size;
            }

            void new_reserve(size_type count)
            {
                // ensure room for count new elements, grow exponentially
                if (unused_capacity() < count)
                {	// need more room, try to get it
                    if (max_size() - size() < count)
                    {
                        _Xlen();
                    }

                    reallocate(grow_to(size() + count));
                }
            }

            std::unique_ptr< t, free_deleter > new_reserve2(size_type count)
            {
                // ensure room for count new elements, grow exponentially
                if (unused_capacity() < count)
                {	// need more room, try to get it
                    if (max_size() - size() < count)
                    {
                        _Xlen();
                    }

                    return reallocate2(grow_to(size() + count));
                }

                return std::unique_ptr< t, free_deleter>();
            }

            void pop_back_n(size_type count)
            {
                auto b = end() - count;
                auto e = end();

                destroy(b, e, std::is_trivially_destructible<t>());
                m_size -= count;
            }

        public:

            reloc_vector() : m_data(nullptr), m_size(0), m_capacity(0)
            {

            }

            reloc_vector(size_type count)
            {
                resize(count);
            }

            reloc_vector(size_type count, const value_type& val)
            {
                resize(count, val);
            }

            reloc_vector(const this_type& right) : m_data(nullptr), m_size(0), m_capacity(0)
            {
                resize(right.size());
                std::copy( std::begin(right), std::end(right), begin() );
            }

            reloc_vector(iterator first, iterator last)
            {
                resize(last - first);
                std::copy(first, last, begin());
            }

            reloc_vector(this_type&& right) noexcept
            {
                m_data = std::move(right.m_data);
                m_size = std::move(right.m_size);
                m_capacity = std::move(right.m_capacity);

                right.m_data = nullptr;
                right.m_size = 0;
                right.m_capacity = 0;
            }

            reloc_vector(const load_context& c)
            {
                //make the offset to the real data
                uintptr_t pointer = reinterpret_cast<intptr_t>(c.base_pointer()) + reinterpret_cast<intptr_t>(this->m_data);
                m_data = reinterpret_cast<t*>(pointer);

                if (is_pod<t>::value)
                {
                    //do nothing
                }
                else
                {
                    //m_size, m_capacity are already setup up
                    for (auto i = 0U; i < m_size; ++i)
                    {
                        auto element = m_data + i;
                        element = placement_new<t>(make_load_context(element));
                    }
                }
                m_enable_allocation = false;
            }

            this_type & operator=(this_type&& right)
            {
                assert(m_enable_allocation);

                m_data                      = std::move(right.m_data);
                m_size                      = std::move(right.m_size);
                m_capacity                  = std::move(right.m_capacity);

                right.m_data                = nullptr;
                right.m_size                = 0;
                right.m_capacity            = 0;


                return *this;
            }

            void push_back(value_type&& value)
            {
                new_reserve(1);
                m_size++;
                back() = std::move(value);
            }

            iterator insert(const_iterator where, this_type&& value)
            {
                return (emplace(where, std::move(value)));
            }

            template<class... values>
            void emplace_back(values&&... v)
            {
                new_reserve(1);
                m_size++;
                back() = std::forward<values...>(v...);
            }

            template<class... values>
            iterator emplace(const_iterator where, values&&... v)
            {
                auto w = where - begin();

                emplace_back(std::forward<values...>(v...));


                std::rotate(begin() + w, end() - 1, end());

                return begin() + w;
            }

            ~reloc_vector() noexcept
            {
                destroy(begin(), end(), std::is_trivially_destructible<t>());

                if (m_enable_allocation)
                {
                    std::free(m_data);
                }
            }

            this_type& operator = (const this_type& right)
            {
                assert(m_enable_allocation);
                if (this != &right)
                {
                    if (right.empty())
                    {
                        clear();
                    }
                    else
                    {
                        resize(right.size());
                        std::copy(right.begin(), right.end(), begin());
                    }
                }
                return *this;
            }

            void reserve(size_type count)
            {
                // determine new minimum length of allocated storage
                if (capacity() < count)
                {	// something to do, check and reallocate
                    if (max_size() < count)
                        _Xlen();
                    reallocate(count);
                }
            }

            size_type capacity() const noexcept
            {
                return m_capacity;
            }

            iterator begin() noexcept
            {
                return m_data;
            }

            const_iterator begin() const noexcept
            {
                return m_data;
            }

            iterator end() noexcept
            {
                return m_data + m_size;
            }

            const_iterator end() const noexcept
            {
                return m_data + m_size;
            }

            const_iterator cbegin() const noexcept
            {
                return m_data;
            }

            const_iterator cend() const noexcept
            {
                return m_data + m_size;
            }

            void shrink_to_fit()
            {
                assert(m_enable_allocation);
                if (unused_capacity() > 0)
                {
                    if (empty())
                    {
                        std::free(m_data);
                        m_data = nullptr;
                        m_size = 0;
                        m_capacity = 0;
                    }
                    else
                    {
                        reallocate(size());
                    }
                }
            }

            void resize(size_type new_size, const value_type& value)
            {
                if (new_size < size())
                {
                    pop_back_n(size() - new_size);
                }
                else if (size() < new_size)
                {
                    assert(m_enable_allocation);
                    std::unique_ptr< t, free_deleter > d(new_reserve2(new_size - size()));
                    t* b = d.get() + size();
                    t* e = d.get() + new_size;

                    std::uninitialized_fill(b, e, value);

                    m_data = d.release();
                    m_size = new_size;
                    m_capacity = new_size;
                }
            }

            void clear()
            {
                for (auto i = begin(); i != end(); ++i)
                {
                    i->~t();
                }

                m_size = 0;
            }

            void resize(size_type new_size)
            {
                resize(new_size, t()); //todo: optimize , skip copy
            }

            size_type size() const noexcept
            {
                return m_size;
            }

            size_type max_size() const noexcept
            {
                return 0xFFFFFFFF;
            }

            bool empty() const noexcept
            {
                return begin() == end();
            }

            const_reference at(size_type position) const
            {
                return *(begin() + position);
            }

            reference at(size_type position)
            {
                return *(begin() + position);
            }

            const_reference operator[](size_type position) const
            {
                return *(begin() + position);
            }

            reference operator[](size_type position)
            {
                return *(begin() + position);
            }

            value_type * data() noexcept
            {
                return m_data;
            }

            const value_type * data() const noexcept
            {
                return m_data;
            }

            reference front()
            {
                return *m_data;
            }

            const_reference front() const
            {
                return *m_data;
            }

            reference back()
            {
                return *(end() - 1);
            }

            const_reference back() const
            {
                return *(end() - 1);
            }

            void push_back(const value_type& value)
            {
                new_reserve(1);
                m_size++;
                back() = value;
            }

            void pop_back()
            {
                pop_back_n(1);
            }

        private:

            t*               m_data;
            size_t           m_size;
            size_t           m_capacity;
            bool             m_enable_allocation = true;

        };


        template <typename t>
        inline bool operator ==(const reloc_vector<t>& lhs, const reloc_vector<t>& rhs)
        {
            if (lhs.size() != rhs.size())
            {
                return false;
            }

            auto s = lhs.size();

            for (auto i = 0U; i < s; ++i)
            {
                if (!( lhs[i] == rhs[i] ) )
                {
                    return false;
                }
            }

            return true;
        }

        template <typename t>
        inline bool operator !=(const reloc_vector<t>& lhs, const reloc_vector<t>& rhs)
        {
            return !(operator==(lhs, rhs));
        }


#if defined(UC_ENGINE)

        template <typename t> using reloc_array = reloc_data<t>;

#elif defined(UC_TOOLS)

        template <typename t> using reloc_array = reloc_vector<t>;

        template <typename t>
        struct member_info_typed_base_traits < reloc_array<t> >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <typename t>  inline const reloc_array<t>* get_reloc_array(const void* root_object, size_t offset)
        {
            uintptr_t ptr = (reinterpret_cast<uintptr_t>(root_object) + offset);
            return reinterpret_cast< const reloc_array<t> * >(ptr);
        }

        template <typename t> struct write_smart_object< reloc_array< t > >
        {
            static void apply(const void* root_object, size_t offset, void* ctx )
            {
                void write_object( const void* root_object, const uc::lip::introspector_base* is, tools_time_writer& w );

                auto w = reinterpret_cast<tools_time_writer*>(ctx);

                auto is     = lip::get_introspector< t >();
                auto root   = get_reloc_array<t>(root_object, offset);

                if ( !root->empty() )
                {
                    w->write_raw_pointer_location(offset);

                    //write size
                    w->copy(offset + sizeof(uintptr_t), sizeof(size_t));

                    if (std::is_pod<t>::value)
                    {
                        size_t    size = root->size() * sizeof(t);
                        w->begin_struct(root->data(), size, std::alignment_of<t>::value );
                        w->copy(0, size);
                        w->end_struct();
                    }
                    else
                    {
                        //now write all members
                        for (auto i = 0U; i < root->size(); ++i)
                        {
                            uintptr_t data = reinterpret_cast<uintptr_t>(root->data() + i);
                            
                            //start new root and write it, if not written
                            void* element = reinterpret_cast<t*> (data);
                            write_object(element, is, *w);
                        }
                    }
                }
            }
        };
#endif

    }

}
