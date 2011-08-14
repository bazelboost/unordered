
// Copyright 2006-2011 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_UNORDERED_TEST_CXX11_ALLOCATOR_HEADER)
#define BOOST_UNORDERED_TEST_CXX11_ALLOCATOR_HEADER

#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <cstddef>

#include "../helpers/memory.hpp"

namespace test
{
    enum allocator_flags
    {
        allocator_false = 0,
        select_copy = 1,
        propagate_swap = 2,
        propagate_assign = 4,
        propagate_move = 8,
        allocator_flags_all = 15,
        no_select_copy = allocator_flags_all - select_copy,
        no_propagate_swap = allocator_flags_all - propagate_swap,
        no_propagate_assign = allocator_flags_all - propagate_assign,
        no_propagate_move = allocator_flags_all - propagate_move
    };
    
    template <int Flag>
    struct copy_allocator_base
    {
        // select_on_copy goes here.
    };

    template <>
    struct copy_allocator_base<allocator_false> {};

    template <int Flag>
    struct swap_allocator_base
    {
        struct propagate_on_container_swap {
            enum { value = Flag != allocator_false }; };
    };

    template <int Flag>
    struct assign_allocator_base
    {
        struct propagate_on_container_copy_assignment {
            enum { value = Flag != allocator_false }; };
    };

    template <int Flag>
    struct move_allocator_base
    {
        struct propagate_on_container_move_assignment {
            enum { value = Flag != allocator_false }; };
    };

    namespace
    {
        // boostinspect:nounnamed
        bool force_equal_allocator_value = false;
    }

    struct force_equal_allocator
    {
        bool old_value_;
    
        explicit force_equal_allocator(bool value)
            : old_value_(force_equal_allocator_value)
        { force_equal_allocator_value = value; }
        
        ~force_equal_allocator()
        { force_equal_allocator_value = old_value_; }
    };

    template <typename T, allocator_flags Flags = propagate_swap>
    struct cxx11_allocator :
        public copy_allocator_base<Flags & select_copy>,
        public swap_allocator_base<Flags & propagate_swap>,
        public assign_allocator_base<Flags & propagate_assign>,
        public move_allocator_base<Flags & propagate_move>
    {
        int tag_;

        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T* pointer;
        typedef T const* const_pointer;
        typedef T& reference;
        typedef T const& const_reference;
        typedef T value_type;

        template <typename U> struct rebind {
            typedef cxx11_allocator<U, Flags> other;
        };

        explicit cxx11_allocator(int t = 0) : tag_(t)
        {
            detail::tracker.allocator_ref();
        }
        
        template <typename Y> cxx11_allocator(
                cxx11_allocator<Y, Flags> const& x)
            : tag_(x.tag_)
        {
            detail::tracker.allocator_ref();
        }

        cxx11_allocator(cxx11_allocator const& x)
            : tag_(x.tag_)
        {
            detail::tracker.allocator_ref();
        }

        ~cxx11_allocator()
        {
            detail::tracker.allocator_unref();
        }

        pointer address(reference r)
        {
            return pointer(&r);
        }

        const_pointer address(const_reference r)
        {
            return const_pointer(&r);
        }

        pointer allocate(size_type n) {
            pointer ptr(static_cast<T*>(::operator new(n * sizeof(T))));
            detail::tracker.track_allocate((void*) ptr, n, sizeof(T), tag_);
            return ptr;
        }

        pointer allocate(size_type n, void const* u)
        {
            pointer ptr(static_cast<T*>(::operator new(n * sizeof(T))));
            detail::tracker.track_allocate((void*) ptr, n, sizeof(T), tag_);
            return ptr;
        }

        void deallocate(pointer p, size_type n)
        {
            // Only checking tags when propagating swap.
            // Note that tags will be tested
            // properly in the normal allocator.
            detail::tracker.track_deallocate((void*) p, n, sizeof(T), tag_,
                (Flags & propagate_swap));
            ::operator delete((void*) p);
        }

        void construct(T* p, T const& t) {
            detail::tracker.track_construct((void*) p, sizeof(T), tag_);
            new(p) T(t);
        }

#if defined(BOOST_UNORDERED_STD_FORWARD_MOVE)
        template<typename... Args> void construct(T* p, Args&&... args) {
            detail::tracker.track_construct((void*) p, sizeof(T), tag_);
            new(p) T(std::forward<Args>(args)...);
        }
#endif

        void destroy(T* p) {
            detail::tracker.track_destroy((void*) p, sizeof(T), tag_);
            p->~T();
        }

        size_type max_size() const {
            return (std::numeric_limits<size_type>::max)();
        }

        // When not propagating swap, allocators are always equal
        // to avoid undefined behaviour.
        bool operator==(cxx11_allocator const& x) const
        {
            return force_equal_allocator_value || (tag_ == x.tag_);
        }

        bool operator!=(cxx11_allocator const& x) const
        {
            return !(*this == x);
        }
    };

    template <typename T, allocator_flags Flags>
    bool equivalent_impl(
            cxx11_allocator<T, Flags> const& x,
            cxx11_allocator<T, Flags> const& y,
            test::derived_type)
    {
        return x.tag_ == y.tag_;
    }

    template <typename T, allocator_flags Flags>
    struct is_propagate_on_swap<cxx11_allocator<T, Flags> >
        : bool_type<(bool)(Flags & propagate_swap)> {};
    template <typename T, allocator_flags Flags>
    struct is_propagate_on_assign<cxx11_allocator<T, Flags> >
        : bool_type<(bool)(Flags & propagate_assign)> {};
    template <typename T, allocator_flags Flags>
    struct is_propagate_on_move<cxx11_allocator<T, Flags> >
        : bool_type<(bool)(Flags & propagate_move)> {};
}

#endif
