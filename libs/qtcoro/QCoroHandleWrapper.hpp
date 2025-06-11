
#pragma once

#include <QObject>
#include <coroutine>

class QCoroHandleWrapper  : public QObject
{
    Q_OBJECT

public:
    QCoroHandleWrapper()
        : suspended(std::noop_coroutine())
    {}

public Q_SLOTS:
    void run()
    {
        suspended.resume();
    }

public:
    std::coroutine_handle<> suspended;
};
