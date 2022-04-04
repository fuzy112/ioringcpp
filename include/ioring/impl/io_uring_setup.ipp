#ifndef IORING_IMPL_IO_URING_SETUP_IPP
#define IORING_IMPL_IO_URING_SETUP_IPP

#include <ioring/io_uring_setup.hpp>

#include <sys/syscall.h>
#include <unistd.h>

namespace ioring
{

    int io_uring_setup(unsigned entries, struct io_uring_params *p)
    {
        return (int)syscall(__NR_io_uring_setup, entries, p);
    }

}

#endif /* IORING_IMPL_IO_URING_SETUP_IPP */
