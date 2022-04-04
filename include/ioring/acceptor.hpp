#ifndef IORING_ACCEPTOR_HPP
#define IORING_ACCEPTOR_HPP

#include <ioring/stream_socket.hpp>

namespace ioring
{

    class acceptor : public stream_socket
    {
    public:
        explicit acceptor(uring &ring)
            : stream_socket(ring)
        {
        }

        void listen(int n)
        {
            if (::listen(this->native_handle(), n) < 0)
                throw std::system_error(errno, std::system_category(), __func__);
        }

        template <typename Socket, typename Endpoint, typename Handler>
        void async_accept(Socket &peer, Endpoint &endpoint, Handler &&handler);

        template <typename Socket, typename Handler>
        void async_accept(Socket &peer, Handler &&handler);

        template <typename Handler>
        void async_accept(Handler &&peer);
    };

    template <typename Socket, typename Endpoint, typename Handler>
    void acceptor::async_accept(Socket &peer, Endpoint &endpoint, Handler &&handler)
    {
        struct accept_op
        {
            accept_op(Socket &sock, Handler &&handler)
                : sock_(sock), handler_(std::forward<Handler>(handler))
            {
            }

            void operator()(io_uring_cqe *cqe)
            {
                if (cqe->res < 0)
                {
                    handler_(std::error_code(-cqe->res, std::system_category()));
                }
                else
                {
                    this->sock_.assign(cqe->res);
                    handler_(std::error_code());
                }
            }

            Socket &sock_;
            typename std::decay<Handler>::type handler_;
        };

        this->get_uring().submit([&](io_uring_sqe *sqe)
                                 {
                                         memset(sqe, 0, sizeof(*sqe));

                                         sqe->opcode = IORING_OP_ACCEPT;
                                         sqe->fd = this->native_handle();
                                         sqe->addr = reinterpret_cast<__u64>(endpoint.get());
                                         sqe->addr2 = reinterpret_cast<__u64>(&endpoint.size());

                                         sqe->user_data = wrapped_operation<accept_op>::create(peer, std::forward<Handler>(handler)); });
    }

    template <typename Socket, typename Handler>
    void acceptor::async_accept(Socket &peer, Handler &&handler)
    {
        generic_endpoint *endpoint = new generic_endpoint();

        this->async_accept(peer, *endpoint, [h = std::forward<Handler>(handler), endpoint](std::error_code ec)
                           {
                delete endpoint;
                h(ec); });
    }

    // template <typename Handler>
    // void async_accept(Handler &&peer)
    // {

    // }

}

#endif /* IORING_ACCEPTOR_HPP */
