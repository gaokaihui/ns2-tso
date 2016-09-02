#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/enhanced-dynamic-threshold.cc,v 1.0 2014/04/10 20:00:00 haldar Exp $ (LBL)";
#endif

#include "enhanced-dynamic-threshold.h"

EDT* EDT::all_queue_[MAX_BUFF_NUM][MAX_PORT] = {NULL};
int EDT::uncontroll_num_[MAX_BUFF_NUM] = {0};

static class EDTClass : public TclClass {
 public:
	EDTClass() : TclClass("Queue/EDT") {}
	TclObject* create(int, const char*const*) {
		return (new EDT);
	}
} class_enhanced_dynamic_threshold;


EDT::EDT() : SharedMemory() {
	bind("timer1_", &timer1_);
	bind("timer2_", &timer2_);
	bind("counter1_", &counter1_cn_);
	bind("counter2_", &counter2_cn_);
	counter1_ = 0;
	counter2_ = 0;
	trigger_time1_ = now();
	trigger_time2_ = -1;
	edt_state_ = EDT_CONTROL;
}

int EDT::command(int argc, const char*const* argv) 
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "attach-buffer") == 0) {
			int id = atoi(argv[2]);
			if (id < 0 || id >= MAX_BUFF_NUM) {
				if (log_level_ >= LOG_ERROR)
					tcl.evalf("puts \"Fail to attach buffer: wrong buffer id: %d\"", id);
				return (TCL_ERROR);
			} else {
				buffer_id_ = id;
				queue_id_ = SharedMemory::shared_buffer_qnum_[id]++;
				EDT::all_queue_[id][queue_id_] = this;
				if (log_level_ >= LOG_DEBUG)
					tcl.evalf("puts \"Attach port %d to buffer %d\"", queue_id_, id);
				return (TCL_OK);
			}
		}
	}
	return Queue::command(argc, argv);
}

int EDT::adj_counters() {
	double now_time = now();
	double elasped_time1 = now_time - trigger_time1_;
	double elasped_time2 = now_time - trigger_time2_;

	// adjust timer1: the traffic is not bursty
	if(edt_state_ == EDT_CONTROL && elasped_time1 >= timer1_) {
		counter1_ = 0;
		counter2_ = 0;
		trigger_time1_ = now_time;
	}

	// adjust timer2: timeout, change to controlled state
	if(edt_state_ == EDT_UNCONTROL && elasped_time2 >= timer2_) {
		to_controlled();
	}

	return (trigger_time2_ >= 0 ? 0 : 1);
}

/*
 * drop-tail
 */
void EDT::printque(char pre) {
	int occupancy = SharedMemory::shared_buffer_occupancy_[buffer_id_];
	int buffer_size = SharedMemory::shared_buffer_size_[buffer_id_];
	// <flag> <time in s> <queue id> <queue length in pkts> <occupied buffer size in pkts>
	// <buffer utilization> <number of ports in uncontrolled state>
	printf(
			"%c %f %d %d %d %.3f %d\n",
			pre, now(), queue_id_,
			q_->length(), occupancy,
			1.0 * occupancy / buffer_size,
			EDT::uncontroll_num_[buffer_id_]);
}

void EDT::enque(Packet* p) {
	adj_counters();
	int buff_enque, buff_size, qlen_enque, threshold;
	if (qib_) {
		buff_enque = SharedMemory::shared_buffer_occu_byte_[buffer_id_] + hdr_cmn::access(p)->size();
		buff_size = SharedMemory::shared_buffer_size_[buffer_id_] * mean_pktsize_;
		qlen_enque = q_->byteLength() + hdr_cmn::access(p)->size();
	} else {
		buff_enque = SharedMemory::shared_buffer_occupancy_[buffer_id_] + 1;
		buff_size = SharedMemory::shared_buffer_size_[buffer_id_];
		qlen_enque = q_->length() + 1;
	}
	threshold = get_threshold();
	if (buff_enque > buff_size) {
		drop(p);
		to_controlled_all();
		if (log_level_ >= LOG_WARNING)
			printque('d');
	} else if (qlen_enque > threshold) {
		drop(p);
		counter1_ = 0;
		counter2_ = 0;
		trigger_time1_ = now();
		if (log_level_ >= LOG_WARNING)
			printque('d');
	} else {
		q_->enque(p);
		SharedMemory::shared_buffer_occupancy_[buffer_id_] += 1;
		SharedMemory::shared_buffer_occu_byte_[buffer_id_] += hdr_cmn::access(p)->size();
		counter1_ = 0;
		/*Change to the uncontrolled state*/
		if (edt_state_ == EDT_CONTROL && ++counter2_ >= counter2_cn_) {
			to_uncontrolled();
		}
		if (log_level_ >= LOG_INFO)
			printque('+');
	}
}

Packet* EDT::deque() {
	adj_counters();
	Packet* pkt = q_->deque();
	SharedMemory::shared_buffer_occupancy_[buffer_id_] --;
	SharedMemory::shared_buffer_occu_byte_[buffer_id_] -= hdr_cmn::access(pkt)->size();
	if (log_level_ >= LOG_INFO)
		printque('-');
	if( pkt != 0 ) {
		counter1_ ++;
		if (edt_state_ == EDT_UNCONTROL) {
			/*Port becomes underloaded. Change to the controlled state*/
			if (counter1_ >= counter1_cn_) {
				to_controlled();
			}
		} else {
			counter2_ --;
			counter2_ = (counter2_ < 0 ? 0 : counter2_);
		}
	}
	return pkt;
}

int EDT::get_threshold() {
	int threshold = SharedMemory::shared_buffer_size_[buffer_id_];
	int free_buffer, buffer_size;
	switch (edt_state_) {
		case EDT_CONTROL:
			if (qib_) {
				free_buffer = SharedMemory::shared_buffer_size_[buffer_id_] * mean_pktsize_ - SharedMemory::shared_buffer_occu_byte_[buffer_id_];
			} else {
				free_buffer = SharedMemory::shared_buffer_size_[buffer_id_] - SharedMemory::shared_buffer_occupancy_[buffer_id_];
			}
			threshold = alpha_ * free_buffer;
			if (log_level_ >= LOG_WARNING && threshold < 0) {
				Tcl::instance().evalf("puts \"Error: threshold < 0 (free_buffer = %d), which should not be happened.\"", free_buffer);
			}
			break;
		case EDT_UNCONTROL:
			buffer_size = SharedMemory::shared_buffer_size_[buffer_id_];
			if (qib_)
				buffer_size *= mean_pktsize_;
			threshold = (1.0 * buffer_size / EDT::uncontroll_num_[buffer_id_]);
			break;
		default:
			break;
	}
	return threshold;
}

void EDT::to_controlled() {
	counter1_ = 0;
	counter2_ = 0;
	trigger_time2_ = -1;
	trigger_time1_ = now();
	if (edt_state_ == EDT_UNCONTROL) {
		edt_state_ = EDT_CONTROL;
		EDT::uncontroll_num_[buffer_id_] --;
	}
}

void EDT::to_uncontrolled() {
	double now_time = now();
	counter1_ = 0;
	counter2_ = 0;
	trigger_time1_ = now_time;
	trigger_time2_ = now_time;
	if (edt_state_ == EDT_CONTROL) {
		edt_state_ = EDT_UNCONTROL;
		EDT::uncontroll_num_[buffer_id_] ++;
	}
}

void EDT::to_controlled_all() {
	for (int i = 0; i < SharedMemory::shared_buffer_qnum_[buffer_id_]; i++) {
		EDT::all_queue_[buffer_id_][i]->to_controlled();
	}
}
