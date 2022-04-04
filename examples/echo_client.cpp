#include <ioring/uring.hpp>
#include <ioring/coroutine.hpp>
#include <ioring/stream_socket.hpp>
#include <ioring/tcp.hpp>
#include <ioring/stream_descriptor.hpp>

#include <iostream>

using namespace ioring;


struct client : coroutine
{
    client(
        tcp::endpoint const &endpoint,
        char *buff,
        std::size_t buff_len,
        stream_descriptor &in,
        stream_socket &sock,
        stream_descriptor &out)
        : endpoint_(endpoint),
          buff_(buff),
          buff_len_(buff_len),
          in_(in),
          sock_(sock),
          out_(out)
    {
        (*this)();
    }

#include <ioring/yield.hpp>
    void operator()(std::error_code ec = {}, std::size_t bytes = 0) const
    {
        if (ec)
        {
            std::cerr << ec << std::endl;
            return;
        }
        reenter(this)
        {
            yield sock_.async_connect(endpoint_, *this);

            for (;;)
            {
                yield in_.async_read_some(mutable_buffer(buff_, buff_len_), *this);
                if (bytes == 0)
                {
                    std::cerr << "1\n";
                    yield break;
                }
                std::cerr << "2\n";
                yield sock_.async_write_some(const_buffer(buff_, bytes), *this);

                yield sock_.async_read_some(mutable_buffer(buff_, buff_len_), *this);

                yield out_.async_write_some(const_buffer(buff_, bytes), *this);
            }
            std::cerr << "3\n";
        }

        std::cerr << "4\n";
    }
#include <ioring/unyield.hpp>

    const tcp::endpoint &endpoint_;
    char *buff_;
    size_t buff_len_;
    stream_descriptor &in_;
    stream_socket &sock_;
    stream_descriptor &out_;
};

int main()
{
    uring ring(20);

    stream_descriptor in(ring);
    in.assign(dup(STDIN_FILENO));

    stream_socket sock(ring);
    sock.open(tcp::v4());

    stream_descriptor out(ring);
    out.assign(dup(STDOUT_FILENO));

    char buff[4096];

    client{
        tcp::endpoint(
            tcp::address_v4(),
            12345),
        buff,
        sizeof(buff),
        in,
        sock,
        out};

    ring.run();
}
