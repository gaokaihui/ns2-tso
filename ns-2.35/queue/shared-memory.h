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

#ifndef ns_shared_memory_h
#define ns_shared_memory_h

// maximum number of shared buffer
#define MAX_BUFF_NUM 100

#define LOG_ERROR 1
#define LOG_WARNING 2
#define LOG_INFO 3
#define LOG_DEBUG 5

#include <string.h>
#include "queue.h"
#include "config.h"

class SharedMemory : public Queue {
  public:
	SharedMemory();
	~SharedMemory() {
		delete q_;
	}
  protected:
	virtual int command(int argc, const char*const* argv); 
	void enque(Packet*);
	Packet* deque();
	int get_threshold(); // get threshold of this output queue
	void printque(char pre);
	inline double now() {
		return Scheduler::instance().clock();
	}

	PacketQueue *q_;	/* underlying FIFO queue */
	int qib_;       	/* bool: queue measured in bytes? */
	int mean_pktsize_;	/* configured mean packet size in bytes */
	int buffer_id_; // id of shared buffer
	int queue_id_; // id of this queue in the shared buffer
	int dt_enable_; // Use Dynamic Threshold as buffer management policy?
	double alpha_;
	// LOG_ERROR: only show error information
	// LOG_WARNING: show error and info information, e.g., packet dropping is in this scope
	// LOG_INFO: show detailed information, e.g., packet enqueue is in this scope
	// LOG_DEBUG: show detailed information
	// For example, if you only want to see packete dropping event, set log_level_ to LOG_WARNING
	// If you only want to see both packete dropping and enqueue event, set log_level_ to LOG_INFO
	int log_level_;
	static int shared_buffer_size_[MAX_BUFF_NUM]; // shared buffer size, in packets
	static int shared_buffer_occupancy_[MAX_BUFF_NUM]; // shared buffer occupancy, in packets
	static int shared_buffer_occu_byte_[MAX_BUFF_NUM]; // shared buffer occupancy, in Bytes
	static int shared_buffer_qnum_[MAX_BUFF_NUM]; // # of queues attached with each shared buffer
};

#endif
