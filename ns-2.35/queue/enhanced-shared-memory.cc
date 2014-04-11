/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/enhanced-shared-memory.cc,v 1.0 2014/04/10 20:00:00 haldar Exp $ (LBL)";
#endif

#include "enhanced-shared-memory.h"

// initialize the static members
EnhancedSharedMemory* EnhancedSharedMemory::all_queue[MAX_LINK_NUM] = {NULL};
int EnhancedSharedMemory::queue_num = 0;

static class EnhancedSharedMemoryClass : public TclClass {
 public:
	EnhancedSharedMemoryClass() : TclClass("Queue/SharedMemory/EnhancedSharedMemory") {}
	TclObject* create(int, const char*const*) {
		return (new EnhancedSharedMemory);
	}
} class_enhanced_shared_memory;

double EnhancedSharedMemory::adj_counters()
{
	long now_time = getCurrentTime();
	double elasped_time1 = now_time - trigger_time1;
	double elasped_time2 = now_time - trigger_time2;

	// adjust timer1
	if(trigger_time2 < 0 && elasped_time1 >= INIT_TIMER1) {
		counter1 = 0;
		counter2 = 0;
		trigger_time1 = now_time - (elasped_time1 - INIT_TIMER1);
	}

	// adjust timer2
	if(trigger_time2 > 0 && elasped_time2 >= INIT_TIMER2) {
		trigger_time2 = -1; 
		trigger_time1 = now_time;
		counter1 = 0;
		counter2 = 0;
	}

	if(trigger_time2 >= 0)
		return 0;
	return 1;
}

/*
 * drop-tail
 */
void EnhancedSharedMemory::enque(Packet* p)
{
	if (summarystats) {
        Queue::updateStats(q_->length());
	}

	double threshold = adj_counters()?get_threshold(queue_id):BUFFER_SIZE;

	if (get_occupied_mem(queue_id) >= BUFFER_SIZE ||
			q_->length() >= threshold )
	{
		drop(p);
	} else {
		q_->enque(p);
		if ( trigger_time2 < 0 ) {
			counter1 = 0;
			if (++counter2 >= COUNTER2) {
				long now_time = getCurrentTime();
				trigger_time2 = now_time;
				trigger_time1 = now_time;
				counter2 = 0;
			}
		}
	}

}

Packet* EnhancedSharedMemory::deque()
{
    if (summarystats && &Scheduler::instance() != NULL) {
        Queue::updateStats(q_->length());
    }

	adj_counters();

	Packet* result = q_->deque();
	if( (int) result != 0 ) {
		if (trigger_time2 < 0) {
			if (++counter1 >= COUNTER1) {
				counter1 = 0;
				counter2 = 0;
			}
		}
	}
	return result;
}

int EnhancedSharedMemory::get_occupied_mem(int id)
{
	int start = (id / SWITCH_PORTS) * SWITCH_PORTS;
	double occupy = 0;
	for(int i = start; i < start + SWITCH_PORTS && i < queue_num; i++ )
	{
		occupy += all_queue[i] -> q_->length();
	}
	return occupy;
}

double EnhancedSharedMemory::get_threshold(int id)
{
	double threshold = get_occupied_mem(id);
	threshold = ALPHA * (BUFFER_SIZE - threshold);
	return threshold > 0 ? threshold : 0;
}
