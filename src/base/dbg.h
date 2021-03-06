/*
 * This file is part of bino, a 3D video player.
 *
 * Copyright (C) 2006-2007, 2009-2011
 * Martin Lambers <marlam@marlam.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DBG_H
#define DBG_H

#include <cassert>

#include "str.h"

#undef assert
#ifdef NDEBUG
# define assert(condition)
#else
# include "msg.h"
# define assert(condition) \
    if (!(condition)) \
    { \
        msg::err_txt("%s:%d: %s: Assertion '%s' failed.", \
                __FILE__, __LINE__, __PRETTY_FUNCTION__, #condition); \
        dbg::crash(); \
    }
#endif

#define HERE str::asprintf("%s, function %s, line %d", __FILE__, __func__, __LINE__)

namespace dbg
{
    void init_crashhandler();
    void backtrace();
    void crash();
    void oom_abort();
}

#endif
