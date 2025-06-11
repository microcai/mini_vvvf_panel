
#pragma once

#include <QTimer>
#include "awaitable.hpp"
#include "QCoroHandleWrapper.hpp"

namespace qtcoro
{
    using namespace ucoro;

    using ::coro_start;

    class QCallableWrapper
    {
        Q_OBJECT
    public:
        std::function<void()> callable;

    public Q_SLOTS:
        void run();

    };

    class QtSignalAwaiter
    {

    public:
        constexpr void await_resume() noexcept { }
        constexpr bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            wrapper->suspended = handle;
        }

        QtSignalAwaiter(QObject* obj, const char* signal)
            : wrapper(new QCoroHandleWrapper)
        {
            QObject::connect(obj, signal, wrapper, SLOT(run()), Qt::SingleShotConnection);
        }


    private:
        QCoroHandleWrapper* wrapper;

    };

    static awaitable<void> WaitForQtSignal(QObject* obj, const char* signal)
    {
        co_await QtSignalAwaiter{obj, signal};
    }


    template <typename INT>
    awaitable<void> coro_delay_ms(INT ms)
    {
        co_await callback_awaitable<void>([ms](auto continuation)
        {
            QTimer::singleShot(std::chrono::milliseconds(ms), [continuation=std::move(continuation)]() mutable {
                continuation();
            });
        });
    }

} // namespace qtcoro

