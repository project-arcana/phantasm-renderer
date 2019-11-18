#pragma once

#include <clean-core/array.hh>

namespace pr::backend::detail
{
// Divide ints and round up
// a > 0, b > 0
template <class T = int>
constexpr T int_div_ceil(T a, T b)
{
    return 1 + ((a - 1) / b);
}

struct page_allocator
{
    void initialize(int num_elements, int num_elems_per_page)
    {
        auto const num_pages = int_div_ceil(num_elements, num_elems_per_page);
        this->_page_size = num_elems_per_page;
        _pages = cc::array<int>::filled(num_pages, 0);
    }

    /// allocate a block of the given size, returns the resulting page or -1
    [[nodiscard]] int allocate(int size)
    {
        int const num_pages = int_div_ceil(size, _page_size);

        int num_contiguous_free_pages = 0;
        for (auto i = 0u; i < _pages.size(); ++i)
        {
            auto const page_val = _pages[i];
            if (page_val > 0)
            {
                // allocated block, skip forward
                i += (page_val - 1);
                num_contiguous_free_pages = 0;
            }
            else
            {
                // free block
                ++num_contiguous_free_pages;
                if (num_contiguous_free_pages == num_pages)
                {
                    // contiguous space sufficient, return start of page
                    auto const allocation_start = i - (num_pages - 1);
                    _pages[allocation_start] = num_pages;
                    return allocation_start;
                }
            }
        }

        // no block found
        return -1;
    }

    /// free the given page
    void free(int page)
    {
        if (page >= 0)
            _pages[page] = 0;
    }

    void free_all() { cc::fill(_pages, 0); }

    [[nodiscard]] int get_page_size() const { return _page_size; }

private:
    // pages, each element is a natural number n
    // n > 0: this and the following n-1 pages are allocated
    // each page not allocated is free (free implies 0, but 0 does not imply free)
    cc::array<int> _pages;
    int _page_size; // the amount of elements per page
};


}
