/*
    Copyright (C) 2020 Slava Monich et al.

    This file is part of GTgram.

    GTgram is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GTgram is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GTgram. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GTGRAM_DEBUG_LOG_H
#define GTGRAM_DEBUG_LOG_H

#include <QLoggingCategory>

#ifndef DEBUG_MODULE
#  define DEBUG_MODULE Debug
#endif

#define LOG_CATEGORY__(x) x##Log
#define LOG_CATEGORY_(x) LOG_CATEGORY__(x)
#define LOG_CATEGORY LOG_CATEGORY_(DEBUG_MODULE)
static const QLoggingCategory LOG_CATEGORY("gtgram." QT_STRINGIFY(DEBUG_MODULE));
#define LOG(x) qCDebug(LOG_CATEGORY) << "[" QT_STRINGIFY(DEBUG_MODULE) "]" << x
#define WARN(x) qCWarning(LOG_CATEGORY) << "[" QT_STRINGIFY(DEBUG_MODULE) "]" << x

// No VERBOSE in release build
#ifndef VERBOSE
#  if defined (QT_DEBUG) || defined (DEBUG)
#    define VERBOSE(x) LOG(x)
#  else
#    define VERBOSE(x)
#  endif
#endif

#endif // GTGRAM_DEBUG_LOG_H
