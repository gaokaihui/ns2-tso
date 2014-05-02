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
	EnhancedSharedMemoryClass() : TclClass("Queue/EnhancedSharedMemory") {}
	TclObject* create(int, const char*const*) {
		return (new EnhancedSharedMemory);
	}
} class_enhanced_shared_memory;

int EnhancedSharedMemory::adj_counters()
{
	double now_time = NOW_TIME;
	double elasped_time1 = now_time - trigger_time1;
	double elasped_time2 = now_time - trigger_time2;

	// adjust timer1
	if(trigger_time2 < 0 && elasped_time1 >= INIT_TIMER1) {
		counter1 = 0;
		counter2 = 0;
		trigger_time1 = now_time - (elasped_time1 - elasped_time1 / INIT_TIMER1 * INIT_TIMER1);
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
	double threshold = adj_counters()?get_threshold(queue_id):BUFFER_SIZE;

	int is_drop = 0;
	if (get_occupied_mem(queue_id) >= BUFFER_SIZE ||
			q_->length() >= threshold ) {
		drop(p);
		printf("d %f %d %d\n", NOW_TIME/1000.0, queue_id, get_occupied_mem(queue_id));
		is_drop = 1;
		// if (queue_id == 0)
		//	printf("time:%.8f dropped counter1:%d counter2:%d trigger_time1:%.2f trigger_time2: %.2f\n", NOW_TIME, counter1, counter2, trigger_time1, trigger_time2);
	} else {
		// if (queue_id == 0)
		//	printf("time:%.8f enqueue counter1:%d counter2:%d trigger_time1:%.2f trigger_time2: %.2f\n", NOW_TIME, counter1, counter2, trigger_time1, trigger_time2);
		q_->enque(p);
	}

	if ( trigger_time2 < 0 ) {
		if (!is_drop) {
			counter2 = counter2 - counter1;
			counter2 = counter2 < 0 ? 0 : counter2;
			counter1 = 0;
			if (++counter2 >= COUNTER2) {
				long now_time = NOW_TIME;
				trigger_time2 = now_time;
				trigger_time1 = now_time;
				counter2 = 0;
			}
		} else {
			counter1 = 0;
			counter2 = 0;
		}
	}

}

Packet* EnhancedSharedMemory::deque()
{
	adj_counters();

	Packet* result = q_->deque();
	if( result != 0 ) {
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
	int occupy = 0;
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
