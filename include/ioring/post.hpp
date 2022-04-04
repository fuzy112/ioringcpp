#ifndef IORING_POST_HPP
#define IORING_POST_HPP

#include <ioring/uring.hpp>

#include <cstring>
#include <system_error>

namespace ioring
{

    template <typename Handler>
    struct post_op
    {
        post_op(uring &ring, Handler &&h)
            : ring(ring),
              handler(std::forward<Handler>(h))
        {
        }

        void operator()(io_uring_cqe *cqe)
        {
            std::error_code ec(-cqe->res, std::system_category());
            handler(ec);
        }

        uring &ring;
        typename std::decay<Handler>::type handler;
    };

    template <typename Handler>
    void post(uring &ring, Handler &&h)
    {
        ring.submit([&](io_uring_sqe *sqe)
                    {
                memset(sqe, 0, sizeof(*sqe));
                sqe->opcode = IORING_OP_NOP;
                sqe->user_data = wrapped_operation<post_op<Handler>>::create(
                    ring,
                    std::forward<Handler>(h)); });
    }

}

#endif /* IORING_POST_HPP */
