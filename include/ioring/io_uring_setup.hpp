#ifndef IORING_IO_URING_SETUP_HPP
#define IORING_IO_URING_SETUP_HPP

#include <ioring/config.hpp>

#include <linux/io_uring.h>

namespace ioring
{

    IORING_DECL int io_uring_setup(unsigned entries, struct io_uring_params *p);
}

#include <ioring/impl/io_uring_setup.ipp>

#endif /* IORING_IO_URING_SETUP_HPP */
