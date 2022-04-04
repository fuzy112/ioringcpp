#ifndef IORING_DESCRIPTOR_HPP
#define IORING_DESCRIPTOR_HPP

#include <ioring/uring.hpp>
#include <ioring/post.hpp>

namespace ioring
{

    class descriptor
    {
    public:
        explicit descriptor(uring &ring)
            : ring_(ring), fd_(-1) {}

        ~descriptor()
        {
            if (fd_ > -1)
            {
                ::close(fd_);
            }
        }

        void assign(int fd)
        {
            if (fd_ > -1)
            {
                ::close(fd_);
            }
            fd_ = fd;
        }

        int native_handle() const noexcept
        {
            return fd_;
        }

        bool is_open() const noexcept
        {
            return fd_ > -1;
        }

        uring &get_uring() const noexcept
        {
            return ring_;
        }

        template <typename Handler>
        void async_close(Handler &&h)
        {
            struct close_op
            {
                close_op(descriptor &desc, Handler &&h)
                    : desc_(desc),
                      handler_(std::forward<Handler>(h))
                {
                }

                void operator()(io_uring_cqe *cqe)
                {
                    desc_.fd_ = -1;

                    std::error_code ec = {-cqe->res, std::system_category()};
                    handler_(ec);
                }

                descriptor &desc_;
                typename std::decay<Handler>::type handler_;
            };

            ring_.submit([&](io_uring_sqe *sqe)
                         {
                    memset(sqe, 0, sizeof(*sqe));
                    sqe->opcode = IORING_OP_CLOSE;
                    sqe->fd = fd_;
                    sqe->user_data = wrapped_operation<close_op>::create(*this, std::forward<Handler>(h)); });
        }

    private:
        uring &ring_;
        int fd_;
    };

}

#endif /* IORING_DESCRIPTOR_HPP */
