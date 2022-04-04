#ifndef IORING_IO_URING_ENTER_HPP
#define IORING_IO_URING_ENTER_HPP

#include <ioring/config.hpp>

#include <linux/io_uring.h>

namespace ioring
{

    IORING_DECL int io_uring_enter(int ring_fd, unsigned int to_submit,
                                   unsigned int min_complete, unsigned int flags);

} // namespace ioring

#include <ioring/impl/io_uring_enter.ipp>

#endif /* IORING_IO_URING_ENTER_HPP */
