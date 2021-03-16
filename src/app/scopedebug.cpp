#include "app/scopedebug.h"

#ifndef NO_DEBUG_CALL_TRACE

static uint thread_local stackLevel = 0;

Scope::Scope(const char *fn, const char *s, const char *e) :
    func(fn),
    start(s),
    end(e)
{
    qDebug().nospace().noquote()
            << start << QByteArray(" ").repeated(stackLevel) << ">>> " << func << end;
    stackLevel++;
}

Scope::~Scope()
{
    stackLevel--;
    qDebug().nospace().noquote()
            << start << QByteArray(" ").repeated(stackLevel) << "<<< " << func << end;
}

ScopeTimer::ScopeTimer(const char *fn, const char *s, const char *e) :Scope(fn, s, e)
{
    timer.start();
}

ScopeTimer::~ScopeTimer()
{
    qDebug() << "TOOK" << timer.elapsed() << "ms";
}

#endif
