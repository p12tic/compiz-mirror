/*
 * Compiz configuration system library
 *
 * Copyright (C) 2007  Dennis Kasprzyk <onestone@beryl-project.org>
 * Copyright (C) 2007  Danny Baumann <maniac@beryl-project.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _CCS_DEFS_H
#define _CCS_DEFS_H

#include <stddef.h>  /* for NULL */

#ifdef  __cplusplus
# define COMPIZCONFIG_BEGIN_DECLS  extern "C" {
# define COMPIZCONFIG_END_DECLS    }
#else
# define COMPIZCONFIG_BEGIN_DECLS
# define COMPIZCONFIG_END_DECLS
#endif


COMPIZCONFIG_BEGIN_DECLS

#ifndef Bool
#define Bool int
#endif

#ifndef TRUE
#define TRUE ~0
#endif

#ifndef FALSE
#define FALSE 0
#endif

COMPIZCONFIG_END_DECLS

#endif
