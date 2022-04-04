#ifndef IORING_SOCKET_BASE_HPP
#define IORING_SOCKET_BASE_HPP

#include <ioring/descriptor.hpp>

#include <sys/socket.h>
#include <cassert>

namespace ioring
{

    class generic_endpoint
    {
    public:
        constexpr generic_endpoint() = default;

        template <typename SockAddr>
        generic_endpoint(const SockAddr &addr)
        {
            memcpy(&storage_, &addr, sizeof(addr));
            length_ = sizeof(addr);
        }

        sockaddr *get() noexcept
        {
            return reinterpret_cast<sockaddr *>(&storage_);
        }

        const sockaddr *get() const noexcept
        {
            return reinterpret_cast<const sockaddr *>(&storage_);
        }

        socklen_t &size() noexcept
        {
            return length_;
        }

        const socklen_t &size() const noexcept
        {
            return length_;
        }

    private:
        sockaddr_storage storage_{};
        socklen_t length_{sizeof(sockaddr_storage)};
    };

    class socket_base : public descriptor
    {
    public:
        explicit socket_base(uring &ring) : descriptor(ring) {}

        struct reuse_address
        {
            explicit reuse_address(bool v) noexcept : value_(v)
            {
            }

            explicit operator bool() const noexcept
            {
                return value_;
            }

            static constexpr int layer()
            {
                return SOL_SOCKET;
            }

            static constexpr int name()
            {
                return SO_REUSEADDR;
            }

            void *value() noexcept
            {
                return &value_;
            }

            const void *value() const noexcept
            {
                return &value_;
            }

            socklen_t length() const noexcept
            {
                return sizeof(value_);
            }

            void length(socklen_t len)
            {
                assert(len == sizeof(value_));
            }

            int value_;
        };

        template <typename SettableOption>
        void set_option(const SettableOption &option)
        {
            if (::setsockopt(this->native_handle(), option.layer(), option.name(),
                             option.value(), option.length()) < 0)
            {
                throw std::system_error(errno, std::system_category(), __func__);
            }
        }

        template <typename GettableOption>
        void get_option(GettableOption &option)
        {
            socklen_t length = option.length();
            if (::getsockopt(this->native_handle(), option.level(),
                             option.name(), option.value(), &length) < 0)
            {
                throw std::system_error(errno, std::system_category(), __func__);
            }
        }

        enum shutdown_method
        {
            shutdown_both = SHUT_RDWR,
            shutdown_recv = SHUT_RD,
            shutdown_send = SHUT_WR,
        };

        template <typename Handler>
        void async_shutdown(shutdown_method method, Handler &&handler)
        {
            get_uring().submit(
                [&](io_uring_sqe *sqe)
                {
                    memset(sqe, 0, sizeof(*sqe));
                    sqe->opcode = IORING_OP_SHUTDOWN;
                    sqe->fd = this->native_handle();
                    sqe->len = static_cast<__u32>(method);
                    sqe->user_data = wrapped_operation<
                        post_op<typename std::decay<Handler>::type>>::create(get_uring(), std::forward<Handler>(handler));
                });
        }
    };

}

#endif /* IORING_SOCKET_BASE_HPP */
