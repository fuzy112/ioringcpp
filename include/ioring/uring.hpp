#ifndef IORING_URING_HPP
#define IORING_URING_HPP

#include <ioring/config.hpp>

#include <atomic>

#include <linux/io_uring.h>

namespace ioring
{

    struct sq_ring
    {
        std::atomic<__u32> *head;
        std::atomic<__u32> *tail;
        __u32 *ring_mask;
        __u32 *ring_entries;
        std::atomic<__u32> *flags;
        __u32 *dropped;
        __u32 *array;
        io_uring_sqe *sqes;
    };

    struct cq_ring
    {
        std::atomic<__u32> *head;
        std::atomic<__u32> *tail;
        __u32 *ring_mask;
        __u32 *ring_entries;
        __u32 *overflow;
        io_uring_cqe *cqes;
        std::atomic<__u32> *flags;
    };

    struct operation
    {
        void (*complete)(struct io_uring_cqe *cqe);
    };

    template <typename T>
    struct wrapped_operation
        : operation
    {
        template <typename... Args>
        explicit wrapped_operation(Args &&...args)
            : operation{do_complete}, t(std::forward<Args>(args)...)
        {
        }

        static void do_complete(io_uring_cqe *cqe)
        {
            auto self = static_cast<wrapped_operation *>(reinterpret_cast<void *>(cqe->user_data));
            T t2 = std::move(self->t);
            delete self;
            t2(cqe);
        }

        template <typename... Args>
        static __u64 __attribute__((used)) create(Args &&...args)
        {
            auto *op = new wrapped_operation<T>(std::forward<Args>(args)...);
            return reinterpret_cast<__u64>(static_cast<void *>(op));
        }

        T t;
    };

    class uring
    {
    public:
        IORING_DECL explicit uring(int queue_depth);

        IORING_DECL ~uring();

        template <typename F>
        void submit(F &&f)
        {
            __u32 tail = sqring_.tail->load(std::memory_order_relaxed);
            if (tail + 1 == sqring_.head->load(std::memory_order_acquire))
                wait();

            __u32 index = tail & *sqring_.ring_mask;
            io_uring_sqe *sqe = &sqring_.sqes[index];
            f(sqe);
            sqring_.array[index] = index;
            ++tail;

            ++pending_;

            sqring_.tail->store(tail, std::memory_order_release);

            if (sqring_.flags->load(std::memory_order_acquire) & IORING_SQ_NEED_WAKEUP)
                wakeup();
        }

        IORING_DECL void run();

    private:
        IORING_DECL void wakeup();

        IORING_DECL void wait();

        IORING_DECL void wait_complete();

        IORING_DECL unsigned complete();

        int fd_;

        __u32 sq_len_;
        void *sq_ptr_;

        __u32 sqes_len_;
        void *sqes_ptr_;

        __u32 cq_len_;
        void *cq_ptr_;

        sq_ring sqring_;
        cq_ring cqring_;

        __u32 pending_;
    };

}

#include <ioring/impl/uring.ipp>

#endif /* IORING_URING_HPP */
