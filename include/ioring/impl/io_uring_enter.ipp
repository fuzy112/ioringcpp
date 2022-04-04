#ifndef IORING_IMPL_IO_URING_ENTER_IPP
#define IORING_IMPL_IO_URING_ENTER_IPP

#include <ioring/io_uring_enter.hpp>

#include <sys/syscall.h>
#include <unistd.h>

namespace ioring
{

    int io_uring_enter(int ring_fd, unsigned int to_submit,
                       unsigned int min_complete, unsigned int flags)
    {
        return (int)syscall(__NR_io_uring_enter, ring_fd, to_submit, min_complete,
                            flags, (void *)0, 0);
    }

}

#endif /* IORING_IMPL_IO_URING_ENTER_IPP */
