#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tools/logoo.cc,v 1.15 2005/08/26 05:05:30 tomh Exp $ (Xerox)";
#endif

#include <stdlib.h>
#include <math.h>
 
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"


/* implement an on/off source with exponentially distributed on and
 * off times.  parameterized by average burst time, average idle time,
 * burst rate and packet size.
 */

class LOGOO_Traffic : public TrafficGenerator {
 public:
	LOGOO_Traffic();
	virtual double next_interval(int&);
        virtual void timeout();
	// Added by Debojyoti Dutta October 12th 2000
 protected:
	void init();
	double ontime_;   /* average length of burst (sec) */
	double offtime_;  /* average length of idle time (sec) */
	double rate_;     /* send rate during on time (bps) */
	double interval_; /* packet inter-arrival time during burst (sec) */
	double ontime_std_; // the standard deviation of on time
	double offtime_std_; // the standard deviation of off time
	double burstlen_;
	double burstlen_std_;
	unsigned int rem_; /* number of packets left in current burst */

	inline double get_on_avg() {
		return log (burstlen_) - 0.5 * log (1 + pow ( burstlen_std_/burstlen_, 2) );
	}
	
	inline double get_on_std() {
		return pow( log (1 + pow ( burstlen_std_ / burstlen_, 2)), 0.5);
	}
};


static class LOGTrafficClass : public TclClass {
 public:
	LOGTrafficClass() : TclClass("Application/Traffic/LogNormal") {}
	TclObject* create(int, const char*const*) {
		return (new LOGOO_Traffic());
	}
} class_logoo_traffic;

// Added by Debojyoti Dutta October 12th 2000
// This is a new command that allows us to use 
// our own RNG object for random number generation
// when generating application traffic


LOGOO_Traffic::LOGOO_Traffic()
{
	bind_time("burst_time_", &ontime_);
	bind_time("idle_time_", &offtime_);
	bind_bw("rate_", &rate_);
	bind("packetSize_", &size_);
	bind("ontime_std_", &ontime_std_);
	bind("offtime_std_", &offtime_std_);
}

void LOGOO_Traffic::init()
{
    /* compute inter-packet interval during bursts based on
	 * packet size and burst rate.  then compute average number
	 * of packets in a burst.
	 */
	interval_ = (double)(size_ << 3)/(double)rate_;
	burstlen_ = ontime_/interval_;
	burstlen_std_ = ontime_std_/interval_;
	// printf("burst len:%f\n", burstlen_);
	// debug printf("avg: %f, %f, std:%f, %f\n", burstlen_.avg(), Offtime_.avg(), burstlen_.std(), Offtime_.std());
	rem_ = 0;
	if (agent_)
		agent_->set_pkttype(PT_EXP);
}

double LOGOO_Traffic::next_interval(int& size)
{
	double t = interval_;

	if (rem_ == 0) {
		/* compute number of packets in next burst */
		rem_ = int(Random::lognormal(get_on_avg(), get_on_std()) + .5);
		// printf( "rem:%d\n", rem_ );
		/* make sure we got at least 1 */
		if (rem_ == 0)
			rem_ = 1;
		/* start of an idle period, compute idle time */
		t += Random::lognormal(offtime_, offtime_std_);
		// printf( "time:%f\n", t );
	}	
	rem_--;

	size = size_;
	return(t);
}

void LOGOO_Traffic::timeout()
{
	if (! running_)
		return;

	/* send a packet */
	// The test tcl/ex/test-rcvr.tcl relies on the "NEW_BURST" flag being 
	// set at the start of any exponential burst ("talkspurt").  
	if (nextPkttime_ != interval_ || nextPkttime_ == -1) 
		agent_->sendmsg(size_, "NEW_BURST");
	else 
		agent_->sendmsg(size_);
	/* figure out when to send the next one */
	nextPkttime_ = next_interval(size_);
	/* schedule it */
	if (nextPkttime_ > 0)
		timer_.resched(nextPkttime_);
}



