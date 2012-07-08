/*
 * Compiz configuration system library
 *
 * Copyright (C) 2012 Canonical Ltd.
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
 *
 * Authored By: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#ifndef _CCS_OBJECT_H
#define _CCS_OBJECT_H

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSInterface CCSInterface; /* Dummy typedef */
typedef struct _CCSPrivate CCSPrivate; /* Dummy typedef */
typedef struct _CCSObject CCSObject;

typedef void * (*reallocObjectProc) (void *, void *, size_t);
typedef void * (*mallocObjectProc) (void *, size_t);
typedef void * (*callocObjectProc) (void *, size_t, size_t);
typedef void (*freeObjectProc) (void *, void *);

typedef struct _CCSObjectAllocationInterface
{
    reallocObjectProc realloc_;
    mallocObjectProc  malloc_;
    callocObjectProc  calloc_;
    freeObjectProc    free_;
    void              *allocator;
} CCSObjectAllocationInterface;

extern CCSObjectAllocationInterface ccsDefaultObjectAllocator;

struct _CCSObject
{
    CCSPrivate *priv; /* Private pointer for object storage */

    const CCSInterface **interfaces; /* An array of interfaces that this object implements */
    int          *interface_types; /* An array of interface types */
    unsigned int n_interfaces;
    unsigned int n_allocated_interfaces;

    CCSObjectAllocationInterface *object_allocation;

    unsigned int refcnt; /* Reference count of this object */
};

Bool
ccsObjectInit_ (CCSObject *object, CCSObjectAllocationInterface *interface);

#define ccsObjectInit(o, interface) (ccsObjectInit_) (&(o)->object, interface)

Bool
ccsObjectAddInterface_ (CCSObject *object, const CCSInterface *interface, int interface_type);

#define ccsObjectAddInterface(o, interface, type) (ccsObjectAddInterface_) (&(o)->object, interface, type);

Bool
ccsObjectRemoveInterface_ (CCSObject *object, int interface_type);

#define ccsObjectRemoveInterface(o, interface_type) (ccsObjectRemoveInterface_) (&(o)->object, interface_type);

const CCSInterface * ccsObjectGetInterface_ (CCSObject *object, int interface_type);

#define ccsObjectGetInterface(o, interface_type) (ccsObjectGetInterface_) (&(o)->object, interface_type)

#define ccsObjectRef(o) \
    do { ((o)->object).refcnt++; } while (FALSE)

#define ccsObjectUnref(o, freeFunc) \
    do \
    { \
	((o)->object).refcnt--; \
	if (!((o)->object).refcnt) \
	    freeFunc (o); \
    } while (FALSE)

CCSPrivate *
ccsObjectGetPrivate_ (CCSObject *object);

#define ccsObjectGetPrivate(o) (ccsObjectGetPrivate_) (&(o)->object)

void
ccsObjectSetPrivate_ (CCSObject *object, CCSPrivate *priv);

#define ccsObjectSetPrivate(o, priv) (ccsObjectSetPrivate_) (&(o)->object, priv)

void
ccsObjectFinalize_ (CCSObject *object);

#define ccsObjectFinalize(o) (ccsObjectFinalize_) (&(o)->object)

unsigned int
ccsAllocateType ();

#define GET_INTERFACE_TYPE(Interface) \
    ccs##Interface##GetType ()

#define INTERFACE_TYPE(Interface) \
    unsigned int ccs##Interface##GetType () \
    { \
	static unsigned int   type_id = 0; \
	if (!type_id) \
	    type_id = ccsAllocateType (); \
	 \
	return type_id; \
    }

#define GET_INTERFACE(CType, o) (CType *) ccsObjectGetInterface (o, GET_INTERFACE_TYPE(CType))

COMPIZCONFIG_END_DECLS

#endif
