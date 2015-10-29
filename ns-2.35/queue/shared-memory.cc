#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/shared-memory.cc,v 1.0 2014/04/10 20:00:00 haldar Exp $ (LBL)";
#endif

#include "shared-memory.h"

// initialize the static members
SharedMemory* SharedMemory::all_queue[MAX_LINK_NUM] = {NULL};
int SharedMemory::queue_num = 0;

static class SharedMemoryClass : public TclClass {
 public:
	SharedMemoryClass() : TclClass("Queue/SharedMemory") {}
	TclObject* create(int, const char*const*) {
		return (new SharedMemory);
	}
} class_shared_memory;

void SharedMemory::reset()
{
	Queue::reset();
}

int 
SharedMemory::command(int argc, const char*const* argv) 
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


void SharedMemory::printque(char pre)
{
	if (DISQUE < 0) {
		return ;
	} else if (DISQUE == 0 && pre != 'd') {
		return ;
	}
	int quelen = get_occupied_mem(queue_id);
	printf("%c %f %d %d %d %.3f %d\n", pre, NOW_TIME/1000.0, queue_id, all_queue[queue_id] -> q_->length(), quelen, 1.0*quelen/BUFFER_SIZE, 1);
}

void SharedMemory::enque(Packet* p)
{
	if (summarystats) {
                // Queue::updateStats(qib_?q_->byteLength():q_->length());
                Queue::updateStats(q_->length());
	}

	// int qlimBytes = qlim_ * mean_pktsize_;
	// if ((!qib_ && (q_->length() + 1) >= qlim_) ||
  	// (qib_ && (q_->byteLength() + hdr_cmn::access(p)->size()) >= qlimBytes)){
		// if the queue would overflow if we added this packet...
	//	if (drop_front_) { /* remove from head of queue */
	//			q_->enque(p);
	//			Packet *pp = q_->deque();
	//			drop(pp);
	//		} else {
	//			drop(p);
	//		}
	// } else {
	//		q_->enque(p);
	// }
	int quelen = get_occupied_mem(queue_id);
	if (get_occupied_mem(queue_id) >= BUFFER_SIZE ||
			q_->length() >= get_threshold(queue_id) )
	{
		drop(p);
		// drop time queue_id queue length utilization
		printque('d');
	} else {
		q_->enque(p);
		if (DISQUE)
			printque('+');
	}

}

//AG if queue size changes, we drop excessive packets...
void SharedMemory::shrink_queue() 
{
	return ;
	/*
        int qlimBytes = qlim_ * mean_pktsize_;
	if (debug_)
		printf("shrink-queue: time %5.2f qlen %d, qlim %d\n",
 			Scheduler::instance().clock(),
			q_->length(), qlim_);
        while ((!qib_ && q_->length() > qlim_) || 
            (qib_ && q_->byteLength() > qlimBytes)) {
                if (drop_front_) { *//* remove from head of queue */
        /*                Packet *pp = q_->deque();
                        drop(pp);
                } else {
                        Packet *pp = q_->tail();
                        q_->remove(pp);
                        drop(pp);
                }
        }
		*/
}

Packet* SharedMemory::deque()
{
    if (summarystats && &Scheduler::instance() != NULL) {
        // Queue::updateStats(qib_?q_->byteLength():q_->length());
        Queue::updateStats(q_->length());
    }
	int quelen = get_occupied_mem(queue_id);
	if (DISQUE)
		printque('-');
	return q_->deque();
}

void SharedMemory::print_summarystats()
{
	//double now = Scheduler::instance().clock();
		printf("Queue Id: %5d", queue_id);
        printf("True average queue: %5.3f", true_ave_);
		printf("Queue Length: %5d", q_->length());
        // if (qib_)
        //        printf(" (in bytes)");
        printf(" time: %5.3f\n", total_time_);
}

int SharedMemory::get_occupied_mem(int id)
{
	int start = (id / SWITCH_PORTS) * SWITCH_PORTS;
	double occupy = 0;
	for(int i = start; i < start + SWITCH_PORTS && i < queue_num; i++ )
	{
		occupy += all_queue[i] -> q_->length();
	}
	return occupy;
}

double SharedMemory::get_threshold(int id)
{
	double threshold = get_occupied_mem(id);
	threshold = ALPHA * (BUFFER_SIZE - threshold);
	return threshold > 0 ? threshold : 0;
}
