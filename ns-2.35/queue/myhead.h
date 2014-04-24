/* contains all of the shared values*/
#ifndef MY_HEAD_H_
#define MY_HEAD_H_

#define PKT_SIZE 850// B
#define MB_DU 300 // microburst duration
#define REAL_CAPACITY 1000 // Mbps
#define REAL_BUFFER_SIZE 16 //MB

#define MAX_LINK_NUM 100 // maximum number of link
#define SWITCH_PORTS 16 // the number of switch ports
#define ALPHA 1
// measured by number of packets
const int BUFFER_SIZE = REAL_BUFFER_SIZE * 1000000 / PKT_SIZE; 
#define BUFFER_SIZE_ REAL_BUFFER_SIZE * 1000000 / PKT_SIZE
// link capcity, measured by packets/s
// const int CAPACITY = REAL_CAPACITY * 1000000 / (PKT_SIZE * 8);
// #define CAPACITY_ (REAL_CAPACITY * 1000000 / (PKT_SIZE * 8))

#define COUNTER1 3
const int COUNTER2 = (int) ALPHA * BUFFER_SIZE_ / ((1 + ALPHA*SWITCH_PORTS/2.0) * (1 + ALPHA*SWITCH_PORTS/2.0));
#define COUNTER2_  (ALPHA * BUFFER_SIZE_ / ((1 + ALPHA*SWITCH_PORTS/2.0) * (1 + ALPHA*SWITCH_PORTS/2.0)))
// const double INIT_TIMER1 = 1000.0 * COUNTER2_ / CAPACITY_;
const double INIT_TIMER1 = 500 * (1 + ALPHA * SWITCH_PORTS) / ((1 + ALPHA * SWITCH_PORTS / 2.0) * (1 + ALPHA * SWITCH_PORTS / 2.0));
const double INIT_TIMER2 = MB_DU;  // timer in milliseconds
#define NOW_TIME Scheduler::instance().clock() * 1000

#endif
