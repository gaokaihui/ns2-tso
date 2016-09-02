#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/shared-memory.cc,v 1.0 2014/04/10 20:00:00 haldar Exp $ (LBL)";
#endif

#include "shared-memory.h"

// initialize the static members
int SharedMemory::shared_buffer_size_[MAX_BUFF_NUM] = {0};
int SharedMemory::shared_buffer_occupancy_[MAX_BUFF_NUM] = {0};
int SharedMemory::shared_buffer_occu_byte_[MAX_BUFF_NUM] = {0};
int SharedMemory::shared_buffer_qnum_[MAX_BUFF_NUM] = {0};

static class SharedMemoryClass : public TclClass {
 public:
	SharedMemoryClass() : TclClass("Queue/SharedMemory") {}
	TclObject* create(int, const char*const*) {
		return (new SharedMemory);
	}
} class_shared_memory;


SharedMemory::SharedMemory() {
	q_ = new PacketQueue; 
	pq_ = q_;
	queue_id_ = 0;
	bind_bool("summarystats_", &summarystats);
	bind_bool("queue_in_bytes_", &qib_);
	bind("mean_pktsize_", &mean_pktsize_);
	bind("log_level_", &log_level_);
	bind_bool("dt_enable_", &dt_enable_);
	bind("alpha_", &alpha_);
}

void SharedMemory::reset()
{
	Queue::reset();
}

int 
SharedMemory::command(int argc, const char*const* argv) 
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
				queue_id_ = shared_buffer_qnum_[id] ++;
				if (log_level_ >= LOG_DEBUG)
					tcl.evalf("puts \"Attach new queue to buffer %d\"", id);
				return (TCL_OK);
			}
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "set-buffer-size") == 0) {
			int id = atoi(argv[2]);
			int buffer_size = atoi(argv[3]);
			if (id < 0 || id >= MAX_BUFF_NUM) {
				if (log_level_ >= LOG_ERROR)
					tcl.evalf("puts \"Fail to set buffer size: wrong buffer id: %d\"", id);
				return (TCL_ERROR);
			} else if (buffer_size < 0) {
				if (log_level_ >= LOG_ERROR)
					tcl.evalf("puts \"Fail to set buffer size: wrong buffer size: %d\"", buffer_size);
				return (TCL_ERROR);
			} else {
				shared_buffer_size_[id] = buffer_size;
				if (log_level_ >= LOG_DEBUG)
					tcl.evalf("puts \"Set buffer %d's size to %d packets\"", id, buffer_size);
				return (TCL_OK);
			}
		}
	}
	return Queue::command(argc, argv);
}


void SharedMemory::printque(char pre)
{
	int occupancy = shared_buffer_occupancy_[buffer_id_];
	int buffer_size = shared_buffer_size_[buffer_id_];
	printf("%c %f %d %d %.3f %d\n", 
			pre, now(), q_->length(), occupancy, 1.0 * occupancy / buffer_size, 1);
}

void SharedMemory::enque(Packet* p)
{
	if (summarystats) {
		Queue::updateStats(qib_?q_->byteLength():q_->length());
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
	int buff_enque, buff_size, qlen_enque, threshold;
	if (qib_) {
		buff_enque = shared_buffer_occu_byte_[buffer_id_] + hdr_cmn::access(p)->size();
		buff_size = shared_buffer_size_[buffer_id_] * mean_pktsize_;
		qlen_enque = q_->byteLength() + hdr_cmn::access(p)->size();
	} else {
		buff_enque = shared_buffer_occupancy_[buffer_id_] + 1;
		buff_size = shared_buffer_size_[buffer_id_];
		qlen_enque = q_->length() + 1;
	}
	threshold = get_threshold();
	if (buff_enque > buff_size || qlen_enque > threshold) {
		drop(p);
		if (log_level_ >= LOG_WARNING) {
			// drop time queue_id queue length utilization
			printque('d');
		}
	} else {
		q_->enque(p);
		shared_buffer_occupancy_[buffer_id_] += 1;
		shared_buffer_occu_byte_[buffer_id_] += hdr_cmn::access(p)->size();
		if (log_level_ >= LOG_INFO)
			printque('+');
	}

}

int SharedMemory::get_threshold() {
	int threshold;
	if (dt_enable_) {
		int free_buffer;
		if (qib_) {
			free_buffer = shared_buffer_size_[buffer_id_] * mean_pktsize_ - shared_buffer_occu_byte_[buffer_id_];
		} else {
			free_buffer = shared_buffer_size_[buffer_id_] - shared_buffer_occupancy_[buffer_id_];
		}
		threshold = alpha_ * free_buffer;
	} else {
		threshold = shared_buffer_size_[buffer_id_];
		if (qib_) {
			threshold *= mean_pktsize_;
		}
	}
	return threshold;
}


Packet* SharedMemory::deque()
{
    if (summarystats && &Scheduler::instance() != NULL) {
        Queue::updateStats(qib_?q_->byteLength():q_->length());
    }
	Packet* p = q_->deque();
	shared_buffer_occupancy_[buffer_id_] --;
	shared_buffer_occu_byte_[buffer_id_] -= hdr_cmn::access(p)->size();
	if (log_level_ >= LOG_INFO)
		printque('-');
	return p;
}
