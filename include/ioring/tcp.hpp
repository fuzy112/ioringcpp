#ifndef IORING_TCP_HPP
#define IORING_TCP_HPP

#include <ioring/socket_base.hpp>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip6.h>

namespace ioring
{

    struct tcp
    {
        int domain_;
        int type_;
        int protocol_;

        struct address_v4
        {
            static constexpr int domain()
            {
                return AF_INET;
            }

            constexpr address_v4() = default;

            char const *bytes() const noexcept
            {
                return reinterpret_cast<char const *>(&data_);
            }

            char *bytes() noexcept
            {
                return reinterpret_cast<char *>(&data_);
            }

            static constexpr size_t size()
            {
                return sizeof(data_);
            }

            in_addr data_{INADDR_ANY};
        };

        struct address_v6
        {
            static constexpr int domain()
            {
                return AF_INET6;
            }

            constexpr address_v6() = default;

            char const *bytes() const noexcept
            {
                return reinterpret_cast<char const *>(&data_);
            }

            char *bytes() noexcept
            {
                return reinterpret_cast<char *>(&data_);
            }

            static constexpr size_t size()
            {
                return sizeof(data_);
            }

            in6_addr data_ = IN6ADDR_ANY_INIT;
        };

        struct address_generic
        {
            template <typename Address>
            address_generic(const Address &addr)
                : famity_(addr.domain())
            {
                memcpy(&addr_, addr.bytes(), addr.size());
                len_ = addr.size();
            }

            int domain() const
            {
                return famity_;
            }

            char *bytes() noexcept
            {
                return reinterpret_cast<char *>(&addr_);
            }

            const char *bytes() const noexcept
            {
                return reinterpret_cast<const char *>(&addr_);
            }

            size_t size() const noexcept
            {
                return len_;
            }

            sa_family_t famity_;
            in6_addr addr_;
            socklen_t len_;
        };

        struct endpoint
        {
            template <typename Address>
            endpoint(Address const &addr, std::uint16_t port)
            {
                data_.ss_family = addr.domain();
                *reinterpret_cast<in_port_t *>(reinterpret_cast<char *>(&data_) + sizeof(sa_family_t)) = htons(port);
                memcpy(reinterpret_cast<char *>(&data_) + sizeof(sa_family_t) + sizeof(in_port_t), addr.bytes(), addr.size());

                len_ = sizeof(sa_family_t) + sizeof(in_port_t) + addr.size();
                if (len_ < sizeof(sockaddr))
                {
                    len_ = sizeof(sockaddr);
                }
            }

            template <typename SockAddr>
            explicit endpoint(const SockAddr &addr)
            {
                memcpy(&data_, &addr, sizeof(addr));
                len_ = sizeof(addr);
            }

            const sockaddr *get() const
            {
                return reinterpret_cast<sockaddr const *>(&data_);
            }

            sockaddr *get()
            {
                return reinterpret_cast<sockaddr *>(&data_);
            }

            socklen_t &size()
            {
                return len_;
            }

            const socklen_t &size() const
            {
                return len_;
            }

            sockaddr_storage data_{};
            socklen_t len_{};
        };

        int domain() const
        {
            return domain_;
        }

        int type() const
        {
            return type_;
        }

        int protocol() const
        {
            return protocol_;
        }

        address_generic loopback_addr_;

        address_generic loopback() const
        {
            return loopback_addr_;
        }

        static const tcp &v4()
        {
            static const tcp instance{
                AF_INET,
                SOCK_STREAM,
                IPPROTO_TCP,
                address_generic(address_v4()),
            };
            return instance;
        }
    };
}

#endif /* IORING_TCP_HPP */
