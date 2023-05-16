/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
