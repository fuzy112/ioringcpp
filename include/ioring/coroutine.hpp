#ifndef IORING_COROUTINE_HPP
#define IORING_COROUTINE_HPP

namespace ioring
{

    struct coroutine
    {
        mutable unsigned __state = 0;
    };

#define IORING_CORO_YIELD                                                           \
    case __LINE__:                                                                  \
        while (*__current_state != __LINE__ && ((*__current_state = __LINE__) > 0)) \
            if (__coro_yield++ != 0)                                                \
            {                                                                       \
                __coro_out = 1;                                                     \
                goto __coro_begin;                                                  \
            }                                                                       \
            else

#define IORING_CORO_REENTER(__coro)                          \
    unsigned *__current_state = &__coro->::ioring::coroutine::__state; \
    unsigned __coro_yield = 0;                               \
    unsigned __coro_out = 0;                                 \
    if (*__current_state == 0)                               \
        goto __coro_first;                                   \
    __coro_begin:                                            \
    if (!__coro_out)                                         \
        switch (*__current_state)                            \
        __coro_first:

}

#endif /* IORING_COROUTINE_HPP */
