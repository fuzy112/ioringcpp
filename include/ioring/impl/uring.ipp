#ifndef IORING_IMPL_URING_IPP
#define IORING_IMPL_URING_IPP

#include <ioring/uring.hpp>
#include <ioring/io_uring_setup.hpp>
#include <ioring/io_uring_enter.hpp>

#include <system_error>
#include <new>

#include <unistd.h>
#include <sys/mman.h>

namespace ioring
{

    template <typename T>
    inline typename std::remove_extent<T>::type *object_at(void *base, std::ptrdiff_t offset) noexcept
    {
        return std::launder(reinterpret_cast<typename std::remove_extent<T>::type *>(reinterpret_cast<char *>(base) + offset));
    }

    uring::uring(int queue_depth)
        : fd_(-1),
          sq_len_(0),
          sq_ptr_(MAP_FAILED),
          sqes_len_(0),
          sqes_ptr_(MAP_FAILED),
          cq_len_(0),
          cq_ptr_(MAP_FAILED),
          pending_(0)
    {
        io_uring_params params = {};
        params.flags = IORING_SETUP_SQPOLL;

        // setup io_uring file descriptor
        fd_ = io_uring_setup(queue_depth, &params);
        if (fd_ < 0)
            goto err_out;

        // map shared memory

        sq_len_ = params.sq_off.array + params.sq_entries * sizeof(__u32);
        sqes_len_ = params.sq_entries * sizeof(io_uring_sqe);
        cq_len_ = params.cq_off.cqes + params.cq_entries * sizeof(io_uring_cqe);

        sq_ptr_ = ::mmap(0, sq_len_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd_, IORING_OFF_SQ_RING);
        if (sq_ptr_ == MAP_FAILED)
            goto err_out;

        sqes_ptr_ = ::mmap(0, sqes_len_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd_, IORING_OFF_SQES);
        if (sqes_ptr_ == MAP_FAILED)
            goto err_out;

        cq_ptr_ = ::mmap(0, cq_len_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd_, IORING_OFF_CQ_RING);
        if (cq_ptr_ == MAP_FAILED)
            goto err_out;

        // setup ring buffer pointers

        sqring_.head =
            ioring::object_at<std::atomic<__u32>>(sq_ptr_, params.sq_off.head);
        sqring_.tail =
            ioring::object_at<std::atomic<__u32>>(sq_ptr_, params.sq_off.tail);
        sqring_.ring_mask =
            ioring::object_at<__u32>(sq_ptr_, params.sq_off.ring_mask);
        sqring_.ring_entries =
            ioring::object_at<__u32>(sq_ptr_, params.sq_off.ring_entries);
        sqring_.flags =
            ioring::object_at<std::atomic<__u32>>(sq_ptr_, params.sq_off.flags);
        sqring_.dropped =
            ioring::object_at<__u32>(sq_ptr_, params.sq_off.dropped);
        sqring_.array =
            ioring::object_at<__u32[]>(sq_ptr_, params.sq_off.array);

        sqring_.sqes =
            ioring::object_at<io_uring_sqe[]>(sqes_ptr_, 0);

        cqring_.head =
            ioring::object_at<std::atomic<__u32>>(cq_ptr_, params.cq_off.head);
        cqring_.tail =
            ioring::object_at<std::atomic<__u32>>(cq_ptr_, params.cq_off.tail);
        cqring_.ring_mask =
            ioring::object_at<__u32>(cq_ptr_, params.cq_off.ring_mask);
        cqring_.ring_entries =
            ioring::object_at<__u32>(cq_ptr_, params.cq_off.ring_entries);
        cqring_.overflow =
            ioring::object_at<__u32>(cq_ptr_, params.cq_off.overflow);
        cqring_.cqes =
            ioring::object_at<io_uring_cqe[]>(cq_ptr_, params.cq_off.cqes);
        cqring_.flags =
            ioring::object_at<std::atomic<__u32>>(cq_ptr_, params.cq_off.flags);

        return;

    err_out:
        std::error_code ec = {errno, std::system_category()};
        if (cq_ptr_ != MAP_FAILED)
            ::munmap(cq_ptr_, cq_len_);
        if (sqes_ptr_ != MAP_FAILED)
            ::munmap(sqes_ptr_, sqes_len_);
        if (sq_ptr_ != MAP_FAILED)
            ::munmap(sq_ptr_, sq_len_);
        if (fd_ > -1)
            ::close(fd_);
        throw std::system_error(ec, __func__);
    }

    uring::~uring()
    {
        if (cq_ptr_ != MAP_FAILED)
            ::munmap(cq_ptr_, cq_len_);
        if (sqes_ptr_ != MAP_FAILED)
            ::munmap(sqes_ptr_, sqes_len_);
        if (sq_ptr_ != MAP_FAILED)
            ::munmap(sq_ptr_, sq_len_);
        if (fd_ > -1)
            ::close(fd_);
    }

    unsigned uring::complete()
    {
        unsigned n = 0;
        __u32 head = cqring_.head->load(std::memory_order_relaxed);
        if (head == cqring_.tail->load(std::memory_order_acquire))
            wait_complete();

        while (head != cqring_.tail->load(std::memory_order_acquire))
        {
            ++n;
            __u32 index = head & *cqring_.ring_mask;
            io_uring_cqe *cqe = &cqring_.cqes[index];
            {
                operation *oper = static_cast<operation *>(
                    reinterpret_cast<void *>(cqe->user_data));

                oper->complete(cqe);
            }
            ++head;
        }
        cqring_.head->store(head, std::memory_order_release);

        pending_ -= n;
        return n;
    }

    void uring::wakeup()
    {
        if (io_uring_enter(fd_, 0, 0, IORING_ENTER_SQ_WAKEUP) < 0)
            throw std::system_error(errno, std::system_category(), __func__);
    }

    void uring::wait()
    {
        if (io_uring_enter(fd_, 0, 0, IORING_ENTER_SQ_WAIT) < 0)
            throw std::system_error(errno, std::system_category(), __func__);
    }

    void uring::wait_complete()
    {
        if (io_uring_enter(fd_, 0, 1, IORING_ENTER_GETEVENTS) < 0)
            throw std::system_error(errno, std::system_category(), __func__);
    }

    void uring::run()
    {
        while (pending_ > 0)
        {
            complete();
        }
    }
}

#endif /* IORING_IMPL_URING_IPP */
