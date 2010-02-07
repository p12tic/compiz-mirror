/**
 *
 * Compiz group plugin
 *
 * queues.cpp
 *
 * Copyright : (C) 2006-2009 by Patrick Niklaus, Roi Cohen, Danny Baumann,
 *				Sam Spilsbury
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
 *	    Sam Spilsbury   <smspillaz@gmail.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **/

#include "group.h"

bool
GroupScreen::dequeue ()
{
    dequeueMoveNotifies ();
    dequeueGrabNotifies ();
    dequeueUngrabNotifies ();
    dequeueWindowNotifies ();
    return false;
}

void
GroupScreen::enqueueMoveNotify (CompWindow *w,
				int	   dx,
				int	   dy,
				bool	   immediate,
				bool	   sync)
{
    PendingMoves *move = new PendingMoves;

    if (!move)
	return;

    move->w  = w;
    move->dx = dx;
    move->dy = dy;

    move->immediate = immediate;
    move->sync      = sync;

    pendingMoves.push_back (move);

    if (!dequeueTimeoutHandle.active ())
    {
	dequeueTimeoutHandle.start ();
    }
}

void
GroupScreen::dequeueMoveNotifies ()
{
    /* Do not enqueue windows as a result of this dequeue */
    queued = true;

    while (!pendingMoves.empty ())
    {
	PendingMoves *move = pendingMoves.front ();

	move->w->move (move->dx, move->dy, move->immediate);

	if (move->sync)
	{
	    PendingSyncs *pendingSync = new PendingSyncs;
	    if (pendingSync)
	    {
		GROUP_WINDOW (move->w);

		gw->needsPosSync = true;
		pendingSync->w   = move->w;

		pendingSyncs.push_back (pendingSync);
	    }
	}

	pendingMoves.pop_front ();
	
	delete move;
    }

    if (!pendingSyncs.empty ())
	dequeueSyncs ();

    queued = false;
}

void
GroupScreen::dequeueSyncs ()
{
    while (!pendingSyncs.empty ())
    {
	PendingSyncs *pendingSync = pendingSyncs.front ();
	
	GROUP_WINDOW (pendingSync->w);
	if (gw->needsPosSync)
	{
	    pendingSync->w->syncPosition ();
	    gw->needsPosSync = false;
	}

	pendingSyncs.pop_front ();

	delete pendingSync;
    }

}

void
GroupScreen::enqueueGrabNotify (CompWindow   *w,
				int          x,
				int          y,
				unsigned int state,
				unsigned int mask)
{
    PendingGrabs *grab = new PendingGrabs;

    if (!grab)
	return;

    grab->w = w;
    grab->x = x;
    grab->y = y;

    grab->state = state;
    grab->mask  = mask;

    pendingGrabs.push_back (grab);

    if (!dequeueTimeoutHandle.active ())
    {
	dequeueTimeoutHandle.start ();
    }
}

void
GroupScreen::dequeueGrabNotifies ()
{
    queued = true;

    while (!pendingGrabs.empty ())
    {
	PendingGrabs *grab = pendingGrabs.front ();

	grab->w->grabNotify (grab->x, grab->y, grab->state, grab->mask);

	pendingGrabs.pop_front ();

	delete grab;
    }

   queued = false;
}

void
GroupScreen::enqueueUngrabNotify (CompWindow *w)
{
    PendingUngrabs *ungrab = new PendingUngrabs;

    if (!ungrab)
	return;

    ungrab->w    = w;

    pendingUngrabs.push_back (ungrab);

    if (!dequeueTimeoutHandle.active ())
    {
	dequeueTimeoutHandle.start ();
    }
}

void
GroupScreen::dequeueUngrabNotifies ()
{
    queued = true;

    while (!pendingUngrabs.empty ())
    {
	PendingUngrabs *ungrab = pendingUngrabs.front ();

	ungrab->w->ungrabNotify ();

	pendingUngrabs.pop_front ();

	delete ungrab;
    }

    queued = false;
}

void
GroupScreen::enqueueWindowNotify (CompWindow *w, CompWindowNotify n)
{
    PendingNotifies *notify = new PendingNotifies;

    if (!notify)	
	return;

    notify->w = w;
    notify->n = n;

    pendingNotifies.push_back (notify);

    if (!dequeueTimeoutHandle.active ())
	dequeueTimeoutHandle.start ();
}

void
GroupScreen::dequeueWindowNotifies ()
{
    queued = true;

    while (!pendingNotifies.empty ())
    {
	PendingNotifies *notify = pendingNotifies.front ();

	GROUP_WINDOW (notify->w);

	switch (notify->n)
	{
	    case CompWindowNotifyMinimize:
		gw->group->minimizeWindows (notify->w, true);
		break;
	    case CompWindowNotifyUnminimize:
		gw->group->minimizeWindows (notify->w, false);
		break;
	    case CompWindowNotifyShade:
		gw->group->shadeWindows (notify->w, true);
		break;
	    case CompWindowNotifyUnshade:
		gw->group->shadeWindows (notify->w, false);
		break;
	    case CompWindowNotifyRestack:
	        if (gw->group && !gw->group->tabBar &&
		    (gw->group != lastRestackedGroup))
	        {
		    if (optionGetRaiseAll ())
		        gw->group->raiseWindows (notify->w);
	        }
	        if (notify->w->managed () && !notify->w->overrideRedirect ())
		    lastRestackedGroup = gw->group;
		break;
	    default:
		break;
	}

	pendingNotifies.pop_front ();

	delete notify;

    }

    queued = false;
}

