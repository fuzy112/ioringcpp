#include <ioring/uring.hpp>
#include <ioring/post.hpp>
#include <ioring/descriptor.hpp>
#include <ioring/stream_descriptor.hpp>
#include <ioring/acceptor.hpp>
#include <ioring/tcp.hpp>

#include <iostream>
#include <memory>
#include <vector>

using namespace ioring;

void fail(const char *what, std::error_code ec)
{
    std::cerr << what << ": " << ec.message() << std::endl;
}

class connection : public std::enable_shared_from_this<connection>
{
public:
    explicit connection(uring &ring)
        : sock_(ring)
    {
    }

    stream_socket &socket()
    {
        return sock_;
    }

    void go()
    {
        buff_.resize(8192);

        sock_.async_read_some(
            mutable_buffer(buff_.data(), buff_.size()),
            [self = shared_from_this()](std::error_code ec, std::size_t bytes_read)
            {
                self->handle_read(ec, bytes_read);
            });
    }

private:
    void handle_read(std::error_code ec, std::size_t bytes_read)
    {
        if (ec)
            return fail("read", ec);

        sock_.async_write_some(
            const_buffer(buff_.data(), bytes_read),
            [self = shared_from_this()](std::error_code ec, std::size_t bytes_written)
            { self->handle_write(ec, bytes_written); });
    }

    void handle_write(std::error_code ec, std::size_t bytes_written)
    {
        if (ec)
            return fail("write", ec);

        sock_.async_read_some(
            mutable_buffer(buff_.data(), buff_.size()),
            [self = shared_from_this()](std::error_code ec, std::size_t bytes_read)
            {
                self->handle_read(ec, bytes_read);
            });
    }

    stream_socket sock_;
    std::vector<char> buff_;
};

class listener : public std::enable_shared_from_this<listener>
{
public:
    explicit listener(uring &ring)
        : acceptor_(ring)
    {
        acceptor_.open(tcp::v4());
        acceptor_.set_option(acceptor::reuse_address(true));
        acceptor_.bind(tcp::endpoint(tcp::address_v4(), 12345));
    }

    void go()
    {
        acceptor_.listen(10);

        conn_ = std::make_shared<connection>(acceptor_.get_uring());
        acceptor_.async_accept(
            conn_->socket(), [self = shared_from_this()](std::error_code ec)
            { self->handle_accept(ec); });
    }

private:
    void handle_accept(std::error_code ec)
    {
        if (ec)
        {
            return fail("accept", ec);
        }

        conn_->go();

        conn_ = std::make_shared<connection>(acceptor_.get_uring());
        acceptor_.async_accept(
            conn_->socket(), [self = shared_from_this()](std::error_code ec)
            { self->handle_accept(ec); });
    }

    acceptor acceptor_;
    std::shared_ptr<connection> conn_;
};

void on_run(uring &ring, std::error_code ec = {})
{
    std::make_shared<listener>(ring)->go();

    auto client = std::make_shared<connection>(ring);
    client->socket().open(tcp::v4());
    client->socket().async_connect(
        tcp::endpoint(tcp::address_v4(), 12345),
        [client](std::error_code ec)
        {
        if (ec)
            return fail("connect", ec);

        client->socket().async_write_some(const_buffer("hello world", 11), 
            [client](std::error_code ec, std::size_t bytes_written) {
            if (ec)
                return fail("write", ec);

            client->go();
        }); });
}

int main()
{
    uring ring(20);

    post(ring, [&ring](std::error_code ec = {})
         { on_run(ring); });

    ring.run();
}
