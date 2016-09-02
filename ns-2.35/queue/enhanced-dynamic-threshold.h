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

#ifndef ns_enhanced_dynamic_threshold_h
#define ns_enhanced_dynamic_threshold_h

#include <string.h>
#include <sys/time.h>
#include "queue.h"
#include "config.h"
#include "scheduler.h"
#include "shared-memory.h"

// maximum number of ports in a switch
#define MAX_PORT 50

#define EDT_CONTROL 0
#define EDT_UNCONTROL 1

class EDT: public SharedMemory {
  public:
	EDT();
	~EDT() {
		delete q_;
	}

  protected:
	virtual int command(int argc, const char*const* argv); 
	void enque(Packet*);
	Packet* deque();
	int get_threshold();
	void to_controlled(); // change to the controlled state
	void to_uncontrolled(); // change to the uncontrolled state
	void to_controlled_all(); // change all of the port to the controlled state
	void printque(char pre);
	int adj_counters(); // adjast counters before enqueue and dequeue, return 0 if timer2 is still running

	int summarystats;
	int counter1_; // value of counter1
	int counter2_; // value of counter2
	double trigger_time1_; // trigger time of timer1 in seconds
	double trigger_time2_; // trigger time of timer2 in seconds
	double timer1_; // initial value of timer 1, in seconds
	double timer2_; // initial value of timer 2, in seconds
	int counter1_cn_; // counting number of counter 1
	int counter2_cn_; // counting number of counter 2
	int queue_id_;
	double alpha_; // parameter of DT
	// 1: display debug info
	// 0: display only when packets are dropped
	// -1: does not display
	int debug_; 
	int edt_state_;

	/* store all of the queues */
	static EDT *all_queue_[MAX_BUFF_NUM][MAX_PORT];
	/* the number of ports in uncontrolled state within a buffer*/
	static int uncontroll_num_[MAX_BUFF_NUM];
};

#endif
