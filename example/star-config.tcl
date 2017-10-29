#!/home/dfshan/Programs/ns-allinone/ns-2.35/ns
#set USAGE "USAGE $argv0 number_of_senders"
if {$argc != 25} {
	puts stderr "Wrong number of arguments $argc"
	exit 0
}
set sim_end [lindex $argv 0]
set snum [lindex $argv 1]
set link_rate [lindex $argv 2]
set rtt [lindex $argv 3]
set source_alg [lindex $argv 4]
set dctcp_enable [lindex $argv 5]
set dctcp_g [lindex $argv 6]
set min_rto [lindex $argv 7]
set my_trace_interval [lindex $argv 8]
set tso_enable [lindex $argv 9]
set ecn_enable [lindex $argv 10]
set queue_size [lindex $argv 11]
set switch_alg [lindex $argv 12]
set q_weight [lindex $argv 13]
set ecn_threshold [lindex $argv 14]
set qlen_sample_interval [lindex $argv 15]
set sent_size [lindex $argv 16]
set trace_dir [lindex $argv 17]
set debug_trace [lindex $argv 18]
# enable sample queue length
set qlen_sample [lindex $argv 19]

set cedm_enable [lindex $argv 20]
set slope_enable [lindex $argv 21]
set s_weight [lindex $argv 22]
set double_threshold [lindex $argv 23]
set threshold2 [lindex $argv 24]

set hnum [expr $snum + 1]
if {$ecn_enable} {
	set K $ecn_threshold
} else {
	set K $queue_size
}

set packet_size 1460
set packet_total 1500

puts "Simulation input:"
puts "Simulation end: ${sim_end}s"
puts "Debug trace: $debug_trace"
puts "Trace dir: $trace_dir"
puts "----------- Topology Settings ----------"
puts "Number of hosts: $hnum"
puts "Link rate: ${link_rate}Gbps"
puts "Overall RTT: [expr 1000 * $rtt]ms"
puts "----------- End HostSettings ----------"
puts "CC algorithm: $source_alg"
puts "DCTCP enable: $dctcp_enable"
puts "DCTCP g: $dctcp_g"
puts "RTO_MIN: [expr 1000 * $min_rto]ms"
puts "my trace interval: [expr 1000 * $my_trace_interval]ms"
puts "TSO enable: $tso_enable"
puts "----------- Switch Settings ----------"
puts "Is ECN enable in switch: $ecn_enable"
puts "Queue size: ${queue_size}pkts = [expr $queue_size * $packet_total/1024]KB"
puts "Switch algorithm: $switch_alg q_weight: $q_weight"
puts "ECN Threshold: ${K}pkts = [expr $K * $packet_total / 1024]KB"
puts "Queue length sample interval: [expr 1000 * $qlen_sample_interval]ms"
puts "Using combined enqueue and dequeue marking: $cedm_enable"
puts "Using slope-based marking: $slope_enable. s_weight: $s_weight"
puts "Using double threshold: $double_threshold. Threshold2: ${threshold2}pkts = [expr $threshold2 * $packet_total / 1024]KB"
puts "----------- Experiment Traffic ----------"
puts "Number of senders: $snum"
puts "Sent size: ${sent_size}B = [expr $sent_size / 1024]KB"

Agent/TCP set ecn_ 1
Agent/TCP set old_ecn_ 1
Agent/TCP set packetSize_ $packet_size
Agent/TCP set window_ 1000000
Agent/TCP set windowInit_ 2
Agent/TCP set slow_start_restart_ true
Agent/TCP set tcpTick_ 0.01
Agent/TCP set minrto_ $min_rto ; # minRTO = 200ms
Agent/TCP set windowOption_ 0
Agent/TCP set rtxcur_init_ $min_rto
if {$queue_size > 12} {
	Agent/TCP set maxcwnd_ [expr $queue_size - 1];
} else {
	Agent/TCP set maxcwnd_ 12;
}

if {$dctcp_enable} {
	Agent/TCP set dctcp_ true
	Agent/TCP set dctcp_g_ $dctcp_g
}

Agent/TCP/FullTcp set segsize_ $packet_size
Agent/TCP/FullTcp set segsperack_ 1
#Agent/TCP/FullTcp set spa_thresh_ 3000
Agent/TCP/FullTcp set spa_thresh_ 0
Agent/TCP/FullTcp set nodelay_ true; # disable Nagle
Agent/TCP/FullTcp set interval_ 0.000006; # delay ACK interval
set fin_num 0
Agent/TCP/FullTcp instproc done_data {} {
	global fin_num snum
	incr fin_num
	if {$fin_num >= $snum} {
		finish
	}
}

if {$tso_enable} {
	Agent/TCP/FullTcp set tso_enable_ true
}

# DROP-TAIL SWITCH SETTING
Queue set limit_ $queue_size
Queue/DropTail set queue_in_bytes_ true
Queue/DropTail set mean_pktsize_ $packet_total
Queue/RED set bytes_ false
Queue/RED set queue_in_bytes_ true
Queue/RED set mean_pktsize_ $packet_total
Queue/RED set setbit_ true
Queue/RED set gentle_ false
Queue/RED set q_weight_ $q_weight
Queue/RED set mark_p_ 1.0
Queue/RED set thresh_ [expr $K]
Queue/RED set maxthresh_ [expr $K]
if {$cedm_enable} {
	Queue/RED set cedm_ true
} else {
	Queue/RED set cedm_ false
}
if {$slope_enable} {
	Queue/RED set slope_ true
	Queue/RED set s_weight_ $s_weight
} else {
	Queue/RED set slope_ false
}
if {$double_threshold} {
	Queue/RED set d_th_ true
	Queue/RED set th2_ $threshold2
} else {
	Queue/RED set d_th_ false
}

DelayLink set avoidReordering_ true


set ns [new Simulator]

#set allfile [open $trace_dir/all-que.tr w]
#$ns trace-all $allfile

set sw [$ns node]
for {set I 0} {$I <= 0} {incr I} {
	set server($I) [$ns node]
	$ns simplex-link $server($I) $sw ${link_rate}Gb [expr $rtt / 4.0] DropTail
	$ns simplex-link $sw $server($I) ${link_rate}Gb [expr $rtt / 4.0] $switch_alg
}
for {set I 1} {$I < $hnum} {incr I} {
	set server($I) [$ns node]
	$ns simplex-link $server($I) $sw ${link_rate}Gb [expr $rtt / 4.0] DropTail
	$ns simplex-link $sw $server($I) ${link_rate}Gb [expr $rtt / 4.0] $switch_alg
}
for {set I 0} {$I < $hnum} {incr I} {
	for {set J 0} {$J < 1} {incr J} {
		if {[string compare -nocase $source_alg "Sack"] == 0} {
			set sink($I,$J) [new Agent/TCP/FullTcp/Sack]
			$sink($I,$J) listen
		} elseif {[string compare -nocase $source_alg "newreno"] == 0} {
			set sink($I,$J) [new Agent/TCP/FullTcp/Newreno]
			$sink($I,$J) listen
		} else {
			set sink($I,$J) [new Agent/TCPSink]
		}
		$ns attach-agent $server($J) $sink($I,$J)
	}
}
for {set I 0} {$I < $hnum} {incr I} {
	for {set J 0} {$J < 1} {incr J} {
		if {[string compare -nocase $source_alg "Sack"] == 0} {
			set tcp($I,$J) [new Agent/TCP/FullTcp/Sack]
		} elseif {[string compare -nocase $source_alg "newreno"] == 0} {
			set tcp($I,$J) [new Agent/TCP/FullTcp/Newreno]
		} else {
			set tcp($I,$J) [new Agent/TCP/FullTcp/Newreno]
		}
		$ns attach-agent $server($I) $tcp($I,$J)
		$ns connect $tcp($I,$J) $sink($I,$J)
		#set ftp($I,$J) [new Application/FTP]
		#$ftp($I,$J) attach-agent $tcp($I,$J)
	}
}

# trace
#[[$ns link $server($snum) $sw] queue] set enque_print_qlen_ true
#[[$ns link $server($snum) $sw] queue] set deque_print_qlen_ true

#$tcp($snum,0) set debug_tso_ true

set qfile(0) [open $trace_dir/que.tr w]
$ns trace-queue $sw $server(0) $qfile(0)
if {$debug_trace} {
	for {set I 1} {$I <= $snum} {incr I} {
		set qfile($I) [open $trace_dir/que-$I.tr w]
		$ns trace-queue $sw $server($I) $qfile($I)
	}
}
if {$qlen_sample} {
	set qlfile [open $trace_dir/qlen.tr w]
	$ns monitor-queue $sw $server(0) $qlfile $qlen_sample_interval
	[$ns link $sw $server(0)] queue-sample-timeout
}

proc finish {} {
	global debug_trace ns qfile qlfile tfp snum
	$ns flush-trace
	close $qfile(0)
	if {$debug_trace} {
		for {set I 1} {$I <= $snum} {incr I} {
			close $qfile($I)
		}
		close $tfp
		close $qlfile
	}
	exit 0
}

proc my_trace {fp} {
	global ns snum tcp my_trace_interval sim_end
	set now [$ns now]
	for {set I 1} {$I <= $snum} {incr I} {
		set cwnd($I) [$tcp($I,0) set cwnd_]
		set dctcp_alpha($I) [$tcp($I,0) set dctcp_alpha_]
	}
	puts -nonewline $fp "$now"
	for {set I 1} {$I <= $snum} {incr I} {
		puts -nonewline $fp " $cwnd($I)"
	}
	for {set I 1} {$I <= $snum} {incr I} {
		puts -nonewline $fp " $dctcp_alpha($I)"
	}
	puts $fp ""
	$ns at [expr $now + $my_trace_interval] "my_trace $fp"
}

# set interval [expr 5 *  0.001 * 0.001]
set duration [expr 100 *  0.001 * 0.001]
set rng [new RNG]
$rng seed 0
set rv_stime [new RandomVariable/Uniform]
$rv_stime use-rng $rng
$rv_stime set min_ 0
$rv_stime set max_ $duration
for {set I 1} {$I <= $snum} {incr I} {
	#$ns at [expr $interval*$I] "$ftp($I,0) send $sent_size"
	set stime [$rv_stime value]
	#$ns at $stime "puts \"$stime: $I to 0\"; $ftp($I,0) send $sent_size"
	$ns at $stime "puts \"$stime: $I to 0\""
	#$ns at $stime "$tcp($I,0) set signal_on_empty_ true"
	$ns at $stime "$tcp($I,0) sendmsg $sent_size DAT_EOF"
}

if {$debug_trace} {
	set tfp [open $trace_dir/mytracefile.tr w]
	$ns at 0 "my_trace $tfp"
}

$ns at $sim_end "finish"
$ns run
