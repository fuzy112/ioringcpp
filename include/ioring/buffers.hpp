#ifndef IORING_BUFFERS_HPP
#define IORING_BUFFERS_HPP

#include <sys/uio.h>

namespace ioring
{

    class mutable_buffer
    {
    public:
        mutable_buffer(void *data = nullptr, size_t size = 0) noexcept
            : data_(data), size_(size)
        {
        }

        void *data() const noexcept
        {
            return data_;
        }

        size_t size() const noexcept
        {
            return size_;
        }

        void operator+=(size_t n) noexcept
        {
            if (n > size_)
                n = size_;

            data_ = reinterpret_cast<char *>(data_) + n;
            size_ -= n;
        }

    private:
        void *data_;
        size_t size_;
    };

    class const_buffer
    {
    public:
        const_buffer(const void *data = 0, size_t size = 0) noexcept
            : data_(data), size_(size)
        {
        }

        const_buffer(mutable_buffer buffer)
            : data_(buffer.data()), size_(buffer.size())
        {
        }

        const void *data() const noexcept
        {
            return data_;
        }

        size_t size() const noexcept
        {
            return size_;
        }

        void operator+=(size_t n) noexcept
        {
            if (n > size_)
                n = size_;

            data_ = reinterpret_cast<const char *>(data_) + n;
            size_ -= n;
        }

    private:
        const void *data_;
        size_t size_;
    };

}

#endif /* IORING_BUFFERS_HPP */
