/*
  PSthreads and UthLib thread libraries
  == Copyright (C) 2007 the OMPi/PSthreads Team
  == Department of Computer Science, University of Ioannina

  This file is part of PSthreads.

  PSthreads is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  PSthreads is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with PSthreads; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*! \file queues.h
    \brief Queues management routines
 */

#ifndef __QUEUES_H__
#define __QUEUES_H__
#define LOCK_QUEUES
/* locks.h should be included before queues.h */

/*
 *	Queues
 */

#define QUEUE_NODE_FIELDS(nodetype)				\
	nodetype *next, *prev			


#define QUEUE_DEFINE(nodetype,queue_type) 			\
typedef union _##queue_type {					\
	struct {						\
		nodetype *head;					\
		nodetype *tail;					\
		_lock_t lock;					\
	} q;							\
	char pad[CACHE_LINE_SIZE];				\
} queue_type


#define _queue_head(_q)		(_q)->q.head
#define _queue_tail(_q)		(_q)->q.tail

#define _queue_init(_q) {					\
	(_q)->q.head = NULL;					\
	(_q)->q.tail = NULL;					\
	_lock_init(&(_q)->q.lock);				\
}

#define _queue_init_bare(_q)	_queue_init(_q)

#define _queue_destroy(_q) {					\
	_lock_destroy(&(_q)->q.lock);				\
}


#define _enqueue_head(_q, _e) {					\
	_lock_acquire(&(_q)->q.lock);				\
	(_e)->prev = NULL;					\
	(_e)->next = (_q)->q.head;				\
	if ((_q)->q.head == NULL) {				\
		(_q)->q.head = (_q)->q.tail = (_e);		\
	}							\
	else {							\
		(_q)->q.head = (_e);				\
		(_e)->next->prev = (_e);			\
	}							\
	_lock_release(&(_q)->q.lock);				\
}


#define _enqueue_tail(_q, _e) {					\
	_lock_acquire(&(_q)->q.lock);				\
	(_e)->next = NULL;					\
	(_e)->prev = (_q)->q.tail;				\
	if ((_q)->q.tail == NULL)	{			\
		(_q)->q.tail = (_q)->q.head = (_e);		\
	}							\
	else {							\
		(_q)->q.tail->next = (_e);			\
		(_q)->q.tail = (_e);				\
	}							\
	_lock_release(&(_q)->q.lock);				\
}

#define _enqueue_bunch_tail(_q, _e1, _e2) {			\
	_lock_acquire(&(_q)->q.lock);				\
	(_e2)->next = NULL;					\
	(_e1)->prev = (_q)->q.tail;				\
	if ((_q)->q.tail == NULL)	{			\
		(_q)->q.head = (_e1);				\
		(_q)->q.tail = (_e2);				\
	}							\
	else {							\
		(_q)->q.tail->next = (_e1);			\
		(_q)->q.tail = (_e2);				\
	}							\
	_lock_release(&(_q)->q.lock);				\
}


#define _dequeue(_q, _thread) {					\
	if ((_q)->q.head != NULL) {				\
		_lock_acquire(&(_q)->q.lock);			\
		(*(_thread)) = (_q)->q.head;			\
		if ((_q)->q.head != NULL) {			\
			(_q)->q.head = (_q)->q.head->next;	\
			if ((_q)->q.head == NULL)		\
				(_q)->q.tail = NULL;		\
			else					\
				(_q)->q.head->prev = NULL;	\
		}						\
		_lock_release(&(_q)->q.lock);			\
	}							\
}


#define _dequeue_end(_q, _thread) {				\
	if ((_q)->q.tail != NULL) {				\
		_lock_acquire(&(_q)->q.lock);			\
		(*(_thread)) = (_q)->q.tail;			\
		if ((_q)->q.tail != NULL) {			\
			(_q)->q.tail = (_q)->q.tail->prev;	\
			if ((_q)->q.tail == NULL)		\
				(_q)->q.head = NULL;		\
			else					\
				(_q)->q.tail->next = NULL;	\
		}						\
		_lock_release(&(_q)->q.lock);			\
	}							\
}


#define _enqueue_head_bare(_q, _e) {				\
	(_e)->prev = NULL;					\
	(_e)->next = (_q)->q.head;				\
	if ((_q)->q.head == NULL) {				\
		(_q)->q.head = (_q)->q.tail = (_e);		\
	}							\
	else {							\
		(_q)->q.head = (_e);				\
		(_e)->next->prev = (_e);			\
	}							\
}


#define _enqueue_tail_bare(_q, _e) {				\
	(_e)->next = NULL;					\
	(_e)->prev = (_q)->q.tail;				\
	if ((_q)->q.tail == NULL)	{			\
		(_q)->q.tail = (_q)->q.head = (_e);		\
	}							\
	else {							\
		(_q)->q.tail->next = (_e);			\
		(_q)->q.tail = (_e);				\
	}							\
}


#define _dequeue_bare(_q, _thread) {				\
	if ((_q)->q.head != NULL) {				\
		(*(_thread)) = (_q)->q.head;			\
		if ((_q)->q.head != NULL) {			\
			(_q)->q.head = (_q)->q.head->next;	\
			if ((_q)->q.head == NULL)		\
				(_q)->q.tail = NULL;		\
			else					\
				(_q)->q.head->prev = NULL;	\
		}						\
	}							\
}


#define _dequeue_end_bare(_q, _thread) {			\
	(*(_thread)) = (_q)->q.tail;				\
	if ((_q)->q.tail != NULL) {				\
		(_q)->q.tail = (_q)->q.tail->prev;		\
		if ((_q)->q.tail == NULL)			\
			(_q)->q.head = NULL;			\
		else						\
			(_q)->q.tail->next = NULL;		\
	}							\
}

#endif
