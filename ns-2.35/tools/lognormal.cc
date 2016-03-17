/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *  
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tools/lognormal.cc,v 1.9 2005/08/26 05:05:31 tomh Exp $ (Xerox)";
#endif
 
#include "random.h"
#include "trafgen.h"
#include <math.h>

/* implement an on/off source with average on and off times taken
 * from a lognormal distribution.  (enough of these sources multiplexed
 * produces aggregate traffic that is LRD).  It is parameterized
 * by the average burst time, average idle time, burst rate, and
 * lognormal shape parameter and packet size.
 */

class LOGOO_Traffic : public TrafficGenerator {
 public:
	LOGOO_Traffic();
	virtual double next_interval(int&);
	int on()  { return on_ ; }
 	// Added by Debojyoti Dutta October 12th 2000
	int command(int argc, const char*const* argv);
protected:
	void init();
	double ontime_;  /* average length of burst (sec) */
	double ontime_std_; /* standard deviation of on period*/
	double offtime_; /* average idle period (sec) */
	double offtime_std_; /* standard deviation of off period*/
	double rate_;    /* send rate during burst (bps) */
	double interval_; /* inter-packet time at burst rate */
	double burstlen_; /* average # packets/burst */
	double burstlen_std_; /* standard deviation of # packets/burst */
	unsigned int rem_; /* number of packets remaining in current burst */
	double l_on_avg_;       /* parameter for lognormal distribution: 
			   * average burst length of corresponding normal distribution.
		           */
	double l_on_std_;       /* parameter for lognormal distribution to compute
			   * standard deviation burst length of corresponding normal distribution.
			   */
	double l_off_avg_;       /* parameter for lognormal distribution: 
			   * average off period of corresponding normal distribution.
		           */
	double l_off_std_;       /* parameter for lognormal distribution to compute
			   * standard deviation off period of corresponding normal distribution.
			   */
	int on_;          /* denotes whether in the on or off state */

	// Added by Debojyoti Dutta 13th October 2000
	RNG * rng_; /* If the user wants to specify his own RNG object */
};


static class LOGOOTrafficClass : public TclClass {
 public:
	LOGOOTrafficClass() : TclClass("Application/Traffic/Lognormal") {}
 	TclObject* create(int, const char*const*) {
		return (new LOGOO_Traffic());
	}
} class_logoo_traffic;


/*
 * Given the average and standard deviation of a lognormal distribution,
 * calculate the average  of its corresponding normal distribution
 */
double avg_normal(double avg, double std) {
	double var = std * std;
	double temp = 1 + var / (avg * avg);
	temp = log(avg) - 0.5 * log(temp);
	return temp;
}

/*
 * Given the average and standard deviation of a lognormal distribution,
 * calculate the standard deviation of its corresponding normal distribution
 */
double std_normal(double avg, double std) {
	double temp = (std / avg) * (std / avg);
	temp = log(1 + temp);
	temp = sqrt(temp);
	return temp;
}

// Added by Debojyoti Dutta October 12th 2000
// This is a new command that allows us to use 
// our own RNG object for random number generation
// when generating application traffic

int LOGOO_Traffic::command(int argc, const char*const* argv){
        
	Tcl& tcl = Tcl::instance();
        if(argc==3){
                if (strcmp(argv[1], "use-rng") == 0) {
			rng_ = (RNG*)TclObject::lookup(argv[2]);
			if (rng_ == 0) {
				tcl.resultf("no such RNG %s", argv[2]);
				return(TCL_ERROR);
			}                        
			return (TCL_OK);
                }
        }
        return Application::command(argc,argv);
}

LOGOO_Traffic::LOGOO_Traffic() : rng_(NULL)
{
	bind_time("burst_time_", &ontime_);
	bind_time("burst_time_std_", &ontime_std_);
	bind_time("idle_time_", &offtime_);
	bind_time("idle_time_std_", &offtime_std_);
	bind_bw("rate_", &rate_);
	bind("packetSize_", &size_);
}

void LOGOO_Traffic::init()
{
	interval_ = (double)(size_ << 3)/(double)rate_;
	burstlen_ = ontime_/interval_;
	burstlen_std_ = ontime_std_/interval_;
	l_on_avg_ = avg_normal(burstlen_, burstlen_std_);
	l_on_std_ = std_normal(burstlen_, burstlen_std_);
	l_off_avg_ = avg_normal(offtime_, offtime_std_);
	l_off_std_ = std_normal(offtime_, offtime_std_);
	rem_ = 0;
	on_ = 0;
	if (agent_)
		/* dfshan: I don't want to change this.
		 * But if one want to, he need to first add a packet type in "common/packet.h".
		 */
		agent_->set_pkttype(PT_PARETO);
}

double LOGOO_Traffic::next_interval(int& size)
{

	double t = interval_;

	on_ = 1;
	if (rem_ == 0) {
		/* compute number of packets in next burst */
		if(rng_ == 0){
			rem_ = int(Random::lognormal(l_on_avg_, l_on_std_) + .5);
		}
		else{
			// Added by Debojyoti Dutta 13th October 2000
			rem_ = int(rng_->lognormal(l_on_avg_, l_on_std_) + .5);
		}
		/* make sure we got at least 1 */
		if (rem_ == 0)
			rem_ = 1;	
		/* start of an idle period, compute idle time */
		if(rng_ == 0){
			t += Random::lognormal(l_off_avg_, l_off_std_);
		}
		else{
			// Added by Debojyoti Dutta 13th October 2000
			t += rng_->lognormal(l_off_avg_, l_off_std_);
		}
		on_ = 0;
	}
	rem_--;

	size = size_;
	return(t);

}

