#ifndef IORING_STREAM_DESCRIPTOR_HPP
#define IORING_STREAM_DESCRIPTOR_HPP

#include <ioring/descriptor.hpp>
#include <ioring/buffers.hpp>

namespace ioring
{

    class stream_descriptor
        : public descriptor
    {
    public:
        explicit stream_descriptor(uring &ring)
            : descriptor(ring)
        {
        }

        template <typename Handler>
        void async_read_some(mutable_buffer buffer, Handler &&handler);

        template <typename Handler>
        void async_write_some(const_buffer buffer, Handler &&handler);
    };

    template <typename Handler>
    void stream_descriptor::async_read_some(mutable_buffer buffer, Handler &&handler)
    {
        struct read_op
        {
            explicit read_op(Handler &&h)
                : handler_(std::forward<Handler>(h))
            {
            }

            void operator()(io_uring_cqe *cqe)
            {
                if (cqe->res < 0)
                {
                    handler_(std::error_code(-cqe->res, std::system_error()),
                             0);
                }
                else
                {
                    handler_(std::error_code(), cqe->res);
                }
            }

            typename std::decay<Handler>::type handler_;
        };

        get_uring().submit([&](io_uring_sqe *sqe)
                           {
                memset(sqe, 0, sizeof(*sqe));
                sqe->opcode = IORING_OP_READ;
                sqe->fd = this->native_handle();
                sqe->addr = reinterpret_cast<__u64>(buffer.data());
                sqe->len = buffer.size();
                sqe->user_data = wrapped_operation<read_op>::create(std::forward<Handler>(handler)); });
    }

    template <typename Handler>
    void stream_descriptor::async_write_some(const_buffer buffer, Handler &&handler)
    {
        struct write_op
        {
            explicit write_op(Handler &&h)
                : handler_(std::forward<Handler>(h))
            {
            }

            void operator()(io_uring_cqe *cqe)
            {
                if (cqe->res < 0)
                {
                    handler_(std::error_code(-cqe->res, std::system_category()),
                             0);
                }
                else
                {
                    handler_(std::error_code(), cqe->res);
                }
            }

            typename std::decay<Handler>::type handler_;
        };

        get_uring().submit([&](io_uring_sqe *sqe)
                           {
                memset(sqe, 0, sizeof(*sqe));
                sqe->opcode = IORING_OP_WRITE;
                sqe->fd = this->native_handle();
                sqe->addr = reinterpret_cast<__u64>(buffer.data());
                sqe->len = buffer.size();
                sqe->user_data = wrapped_operation<write_op>::create(std::forward<Handler>(handler)); });
    }
}

#endif /* IORING_STREAM_DESCRIPTOR_HPP */
