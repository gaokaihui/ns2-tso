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
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/drop-tail.h,v 1.19 2004/10/28 23:35:37 haldar Exp $ (LBL)
 */

#ifndef ns_enhanced_shared_memory_h
#define ns_enhanced_shared_memory_h

#include <string.h>
#include <sys/time.h>
#include "queue.h"
#include "config.h"
#include "shared-memory.h"

#define MAX_LINK_NUM 100 // maximum number of link
#define SWITCH_PORTS 3 // the number of switch ports
#define ALPHA 0.5
#define BUFFER_SIZE 6
#define COUNTER1 1
#define COUNTER2 3
#define INIT_TIMER1 3	// timer in milliseconds
#define INIT_TIMER2 3	// timer in milliseconds

/*
 * A bounded, drop-tail queue
 */
class EnhancedSharedMemory : public SharedMemory {
  public:
	EnhancedSharedMemory() {
		counter1 = 0;
		counter2 = 0;
		trigger_time1 = getCurrentTime();
		trigger_time2 = -1;
	}
	static int get_occupied_mem(int id); // get threshold of queue id
	static double get_threshold(int id); // get threshold of queue id

  protected:
	void enque(Packet*);
	Packet* deque();
	inline long getCurrentTime() {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000 + tv.tv_usec / 1000;
	}
	double adj_counters(); // adjast counters before enqueue and dequeue, return 0 if timer2 is still running
	static EnhancedSharedMemory* all_queue[MAX_LINK_NUM]; /* store all of the queues */
	static int queue_num; /* the number of queues */
	int counter1; // value of counter1
	int counter2; // value of counter2
	long trigger_time1; // trigger time of timer1 in milliseconds
	long trigger_time2; // trigger time of timer2 in milliseconds
};

#endif
