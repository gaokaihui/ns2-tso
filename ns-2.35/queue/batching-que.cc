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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/batching-que.cc,v 1.17 2004/10/28 23:35:37 haldar Exp $ (LBL)";
#endif

#include "batching-que.h"

static class BatchingQueClass : public TclClass {
 public:
	BatchingQueClass() : TclClass("Queue/BatchingQue") {}
	TclObject* create(int, const char*const*) {
		return (new BatchingQue);
	}
} class_batching_que;

void BatchingQue::reset()
{
	Queue::reset();
}

int 
BatchingQue::command(int argc, const char*const* argv) 
{
	if (argc==2) {
		if (strcmp(argv[1], "printstats") == 0) {
			print_summarystats();
			return (TCL_OK);
		}
 		if (strcmp(argv[1], "shrink-queue") == 0) {
 			shrink_queue();
 			return (TCL_OK);
 		}
	}
	if (argc == 3) {
		if (!strcmp(argv[1], "packetqueue-attach")) {
			delete q_;
			if (!(q_ = (PacketQueue*) TclObject::lookup(argv[2])))
				return (TCL_ERROR);
			else {
				pq_ = q_;
				return (TCL_OK);
			}
		}
	}
	return Queue::command(argc, argv);
}

/*
 * drop-tail
 */
void BatchingQue::enque(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double now = s.clock();
	// printf("(debug) enqueue at time %lf qlen: %d uid:%d sending:%d blocked_:%d\n", now, q_->byteLength(), intr_.uid_, sending, blocked_);
	if (summarystats) {
        Queue::updateStats(qib_?q_->byteLength():q_->length());
	}

	int qlimBytes = qlim_ * mean_pktsize_;
	if ((!qib_ && (q_->length() + 1) >= qlim_) ||
  	(qib_ && (q_->byteLength() + hdr_cmn::access(p)->size()) >= qlimBytes)){
		// if the queue would overflow if we added this packet...
		if (drop_front_) { /* remove from head of queue */
			q_->enque(p);
			Packet *pp = q_->deque();
			drop(pp);
		} else {
			drop(p);
		}
	} else {
		q_->enque(p);
	}
}

//AG if queue size changes, we drop excessive packets...
void BatchingQue::shrink_queue() 
{
        int qlimBytes = qlim_ * mean_pktsize_;
	if (debug_)
		printf("shrink-queue: time %5.2f qlen %d, qlim %d\n",
 			Scheduler::instance().clock(),
			q_->length(), qlim_);
        while ((!qib_ && q_->length() > qlim_) || 
            (qib_ && q_->byteLength() > qlimBytes)) {
                if (drop_front_) { /* remove from head of queue */
                        Packet *pp = q_->deque();
                        drop(pp);
                } else {
                        Packet *pp = q_->tail();
                        q_->remove(pp);
                        drop(pp);
                }
        }
}

Packet* BatchingQue::deque()
{
	Scheduler& s = Scheduler::instance();
	double now = s.clock();
	// printf("(debug) dequeue at time %lf qlen: %d uid:%d sending:%d blocked_:%d\n", now, prev_send_time_, q_->byteLength(), intr_.uid_, sending, blocked_);
	if (!sending && (now - prev_send_time_ < timeout_) && q_->byteLength() < batch_size_) {
		// printf("defer sending\n");
		if (intr_.uid_ <= 0) {
			// printf("set schedule\n");
			s.schedule(&qh_, &intr_, timeout_);
		}
		return NULL;
	} else {
		// printf("to deque\n");
		sending = 1;
		prev_send_time_ = now;
		s.cancel(&intr_);
		if (summarystats && &Scheduler::instance() != NULL) {
    	    Queue::updateStats(qib_?q_->byteLength():q_->length());
    	}
		Packet* p = q_->deque();
		if (q_->length() == 0)
			sending = 0;
		// printf("dequeued\n");
		return p;
	}
}

void BatchingQue::print_summarystats()
{
	//double now = Scheduler::instance().clock();
    printf("True average queue: %5.3f", true_ave_);
    if (qib_)
        printf(" (in bytes)");
    printf(" time: %5.3f\n", total_time_);
}
