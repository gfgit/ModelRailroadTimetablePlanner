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

#ifndef SCOPEDEBUG_H
#define SCOPEDEBUG_H


#define SHELL_RESET     "\033[0m"

#define SHELL_RED       "\033[031m"
#define SHELL_GREEN     "\033[032m"
#define SHELL_YELLOW    "\033[033m"
#define SHELL_BLUE      "\033[034m"


#include <QDebug>
#include <QString>
#include <QElapsedTimer>

//#define NO_DEBUG_CALL_TRACE

#ifndef NO_DEBUG_CALL_TRACE

class Scope
{
public:
    Scope(const char *fn, const char *s="", const char* e="");
    ~Scope();

    const char *func, *start, *end;
};


class ScopeTimer : Scope
{
public:
    ScopeTimer(const char *fn, const char *s="", const char* e="");
    ~ScopeTimer();

    QElapsedTimer timer;
};



#   define DEBUG_ENTRY_NAME(name) Scope DBG(name)
#   define DEBUG_ENTRY DEBUG_ENTRY_NAME(__PRETTY_FUNCTION__)
#   define DEBUG_COLOR_ENTRY(color) Scope DBG(__PRETTY_FUNCTION__, color, SHELL_RESET)
#   define DEBUG_IMPORTANT_ENTRY DEBUG_COLOR_ENTRY(SHELL_GREEN)

#   define DEBUG_TIME_ENTRY ScopeTimer DBG(__PRETTY_FUNCTION__)

#else
#   define DEBUG_ENTRY_NAME(name)
#   define DEBUG_ENTRY
#   define DEBUG_COLOR_ENTRY(color)
#   define DEBUG_IMPORTANT_ENTRY
#   define DEBUG_TIME_ENTRY
#endif // NO_DEBUG_CALLTRACE

#endif // SCOPEDEBUG_H
