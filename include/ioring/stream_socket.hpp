#ifndef IORING_STREAM_SOCKET_HPP
#define IORING_STREAM_SOCKET_HPP

#include <ioring/socket_base.hpp>
#include <ioring/buffers.hpp>

namespace ioring
{

    class stream_socket : public socket_base
    {
    public:
        explicit stream_socket(uring &ring)
            : socket_base(ring) {}

        template <typename Protocol>
        void open(const Protocol &protocol)
        {
            int sock = ::socket(protocol.domain(), protocol.type(), protocol.protocol());
            if (sock < 0)
                throw std::system_error(errno, std::system_category(), __func__);
            this->assign(sock);
        }

        template <typename Endpoint>
        void bind(const Endpoint &endpoint)
        {
            if (::bind(this->native_handle(),
                       endpoint.get(), endpoint.size()) < 0)
                throw std::system_error(errno, std::system_category(), __func__);
        }

        template <typename Endpoint, typename Handler>
        void async_connect(const Endpoint &endpoint, Handler &&handler);

        template <typename Handler>
        void async_read_some(mutable_buffer buffer, Handler &&handler);

        template <typename Handler>
        void async_write_some(const_buffer buffer, Handler &&handler);
    };

    template <typename Endpoint, typename Handler>
    void stream_socket::async_connect(const Endpoint &endpoint, Handler &&handler)
    {
        this->get_uring().submit([&](io_uring_sqe *sqe)
                                 {
                    memset(sqe, 0, sizeof(sqe));
                    sqe->opcode = IORING_OP_CONNECT;
                    sqe->fd = this->native_handle();
                    sqe->addr = reinterpret_cast<__u64>(endpoint.get());
                    sqe->off = endpoint.size();

                    sqe->user_data = wrapped_operation<post_op<typename std::decay<Handler>::type>>::
                        create(this->get_uring(), std::forward<Handler>(handler)); });
    }

    template <typename Handler>
    void stream_socket::async_read_some(mutable_buffer buffer, Handler &&handler)
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
                sqe->opcode = IORING_OP_READ;
                sqe->fd = this->native_handle();
                sqe->addr = reinterpret_cast<__u64>(buffer.data());
                sqe->len = buffer.size();
                sqe->off = 0;
                sqe->user_data = wrapped_operation<read_op>::create(std::forward<Handler>(handler)); });
    }

    template <typename Handler>
    void stream_socket::async_write_some(const_buffer buffer, Handler &&handler)
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

        this->get_uring().submit([&](io_uring_sqe *sqe)
                                 {
                memset(sqe, 0, sizeof(*sqe));
                sqe->opcode = IORING_OP_WRITE;
                sqe->fd = this->native_handle();
                sqe->addr = reinterpret_cast<__u64>(buffer.data());
                sqe->len = buffer.size();
                sqe->user_data = wrapped_operation<write_op>::create(std::forward<Handler>(handler)); });
    }
}

#endif /* IORING_STREAM_SOCKET_HPP */
