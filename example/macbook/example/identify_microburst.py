import sys
import random
import math
import os, re
import heapq
import matplotlib
import matplotlib.pyplot as plt
matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

ts_data = {}
qlen_data = {}
qlen_realtime = {}
node_throughput = {}
node_ts_throughput = {}
sum_recv_bytes = 0
last_recv_bytes = 0
interval = 10 #us
last_stat_ts = 0

def getListValueBetweenKey(str):
    p1 = re.compile(r"[/](.*?)[.]", re.S) #get lists between _ _
    arr = re.findall(p1, str)
    return arr[-1]

def handle_recver_throughput(name, link, ts, pkt_size, flow_id, src_addr, dst_addr):
    global node_throughput, sum_recv_bytes, last_recv_bytes, interval, last_stat_ts
    key = name+str(link[1])
    if key not in node_throughput:
        node_throughput[key] = []
        node_ts_throughput[key] = []
        last_stat_ts = ts
    sum_recv_bytes += pkt_size
    if ts - last_stat_ts >= interval/1e6:
        rate = (sum_recv_bytes - last_recv_bytes) * 8 / ((ts - last_stat_ts)*1e6) # Mbps
        last_recv_bytes = sum_recv_bytes
        last_stat_ts = ts
        node_ts_throughput[key].append(ts)
        node_throughput[key].append(rate)

def handle_link_en_dequeue(name, link, event_type, ts, pkt_size):
    global ts_data, qlen_data, qlen_realtime
    key = name+link
    if key not in qlen_realtime:
        qlen_realtime[key] = 0
    if '+' in event_type:
        if key not in qlen_data:
            qlen_data[key] = {"enqueue":[],"dequeue":[]}
            ts_data[key] = {"enqueue":[],"dequeue":[]}
        ts_data[key]["enqueue"].append(ts)
        qlen_data[key]["enqueue"].append(qlen_realtime[key])
        qlen_realtime[key] += pkt_size
    elif '-' in event_type:
        if key not in qlen_data:
            qlen_data[key] = {"enqueue":[],"dequeue":[]}
            ts_data[key] = {"enqueue":[],"dequeue":[]}
        qlen_realtime[key] -= pkt_size
        ts_data[key]["dequeue"].append(ts)
        qlen_data[key]["dequeue"].append(qlen_realtime[key])

def find_max_index(data):
    num = 0
    index = 0
    for i in range(len(data)):
        if data[i] >= num:
            index = i
            num = data[i]
    left_index = index
    while data[left_index] == data[index] and left_index >= 0:
        left_index -= 1
    return left_index+1, index, num
def find_left_min_index(data):
    for i in range(len(data)-1, 0,-1):
        if data[i] <= data[i-1]:
            return i, data[i]
    return 0, data[0]
def find_right_min_index(data):
    for i in range(len(data)-1):
        if data[i] <= data[i+1]:
            return i, data[i]
    return len(data)-1, data[-1]
def judge_extreme_point(data):
    median = int(len(data)/2)
    if data[0] < data[median] and data[-1] < data[median]:
        return 1
    if data[0] > data[median] and data[-1] > data[median]:
        return 1
    return 0

def judge_by_slope(old_index, queue_snapshot, ts_queue_snapshot, now_ts, alpha):
    max_left_index, max_index, max_qlen = find_max_index(queue_snapshot)
    if ts_queue_snapshot[max_index] - ts_queue_snapshot[max_left_index] > 0.000008: # the flat top cannot be aggregated if it > 20us
        max_left_index = max_index
    if max_left_index == 0:
        return 0, 0
    left_min_index, left_min_qlen = find_left_min_index(queue_snapshot[:max_left_index])
    right_min_index, right_min_qlen = find_right_min_index(queue_snapshot[max_index:])
    right_min_index += max_index
    if ts_queue_snapshot[max_left_index] == ts_queue_snapshot[left_min_index]:
        return 0, 0
    if ts_queue_snapshot[right_min_index] == ts_queue_snapshot[max_index]:
        return 0, 0
    slope1 = (max_qlen - left_min_qlen) * 8. / (ts_queue_snapshot[max_left_index] - ts_queue_snapshot[left_min_index])
    slope2 = (max_qlen - right_min_qlen)* 8. / (ts_queue_snapshot[right_min_index] - ts_queue_snapshot[max_index])

    #### judge microburst ####
    C = 10*1e9 #bps link rate
    if slope1 > alpha*C and slope2 > alpha*C:
        return 1, old_index + right_min_index  #avoid multiple microburst signals triggered by one time of queue length jitter
    else:
        return 0, 0

def judge_by_oscillation_times(old_index, queue_snapshot, ts_queue_snapshot, now_ts, th, th2):
    oscillation_length = th
    times = 0
    for i in range(len(queue_snapshot)-oscillation_length+1):
        if judge_extreme_point(queue_snapshot[i:i+oscillation_length]):
            times+=1
    if times >= th2:
        return 1, old_index+len(queue_snapshot)
    else:
        return 0, 0

def identity_microburst(index, key, th, times, wnd):
    global ts_data, qlen_data
    RTT = 0.0001 #0.1ms
    queue_snapshot = []
    ts_queue_snapshot = []
    old_index = index
    now_ts = ts_data[key]["dequeue"][index]
    past_ts = now_ts
    while now_ts - past_ts < wnd*RTT:     #track the past 1.0 * RTT
        if index < 0:
            queue_snapshot.insert(0, 0)
            ts_queue_snapshot.insert(0, 0)
        else:
            queue_snapshot.insert(0, qlen_data[key]["dequeue"][index])
            ts_queue_snapshot.insert(0, ts_data[key]["dequeue"][index])
        index -= 1
        if index < 0:
            past_ts -= 1500 * 8 / (10*1e9)
        else:
            past_ts = ts_data[key]["dequeue"][index]
    #print(len(queue_snapshot))  #83 packets under steady state
    #print(queue_snapshot)
    #### debug ####
    if 0 and now_ts > 0.001 and now_ts < 0.0025:
        plt.figure(figsize=(8,4))
        plt.plot(ts_queue_snapshot, queue_snapshot,label = "dequeue", linestyle= '-',color="#4D72BE")
        plt.legend(loc="best",frameon=False,fontsize=24)
        plt.xlabel('time', fontsize=24)
        plt.ylabel('Qlen (B)', fontsize=24)
        #plt.ylim(0.8, 1.01)
        #plt.xlim(0, 0.005)
        plt.tight_layout(pad=0)
        #plt.show()
        plt.savefig("id_microburst/%s/qlen_%d.png"%(key[:-6],old_index),dpi=300)
        plt.clf()
        plt.close()
    if 0:
        return judge_by_slope(old_index, queue_snapshot, ts_queue_snapshot, now_ts, 0.7)
    else:
        return judge_by_oscillation_times(old_index, queue_snapshot, ts_queue_snapshot, now_ts, th, times)
    

if __name__ == "__main__":
    #file_names = ["no_tso","tso","tso_cedm","tso_min_25","tso_th_60","tso_avg_99"]
    file_names = ["no_tso","tso","tso_cedm"]
    for name in file_names:
        print(name+"_traces/que.tr")
        if "now" in name:
            fp = open("traces/que.tr")
        else:
            fp = open(name+"_traces/que.tr")
        line = fp.readline()
        while line:
            data = line.split(' ')
            event_type = data[0] # + - r d
            ts = float(data[1]) 
            from_node = int(data[2])
            to_node = int(data[3])
            pkt_type = data[4] #tcp
            pkt_size = int(data[5])
            flags = data[6]
            flow_id = int(data[7])
            src_addr = int( float(data[8])*10 )
            dst_addr = int( float(data[9])*10 )
            seq_num = int(data[10])
            pkt_id = int(data[11])

            link = (from_node, to_node)
            if '+' in event_type or '-' in event_type:
                handle_link_en_dequeue(name, str(link), event_type, ts, pkt_size)
            elif 'r' in event_type:
                handle_recver_throughput(name, link, ts, pkt_size, flow_id, src_addr, dst_addr)
            elif 'd' in event_type:
                print("##############Drop packet")
            line = fp.readline()

    #identified microburst
    MB_marked_ts = {}
    MB_marked_qlen = {}
    for name in qlen_data:
        for x in range(4, 13):
            for y in range(2, 7):
                for z in [1, 2, 4, 6, 8, 10]:
                    MB_marked_qlen[name] = []
                    MB_marked_ts[name] = []
                    i = 0
                    mb_num = 0
                    while i < len(qlen_data[name]["dequeue"]):
                        res, new_index = identity_microburst(i, name, x, y, z)
                        if res:
                            mb_num += 1
                            MB_marked_qlen[name].append(qlen_data[name]["dequeue"][i])
                            MB_marked_ts[name].append(ts_data[name]["dequeue"][i])
                            i = new_index + 1
                        else:
                            i += 1
                    print(x, y, z, name, mb_num)
    
    # for name in qlen_data:
    #     plt.figure(figsize=(8,4))
    #     plt.scatter(MB_marked_ts[name], MB_marked_qlen[name], color="brown", marker='x')
    #     plt.plot(ts_data[name]["dequeue"], qlen_data[name]["dequeue"],label = "dequeue", linestyle= '-',color="#4D72BE")
    #     plt.legend(loc="best",frameon=False,fontsize=24)
    #     plt.xlabel('time', fontsize=24)
    #     plt.ylabel('Qlen (B)', fontsize=24)
    #     #plt.ylim(0.8, 1.01)
    #     plt.xlim(0, 0.025)
    #     plt.tight_layout(pad=0)
    #     plt.show()
    #     #plt.savefig("./qlen.png",dpi=300)
    #     plt.clf()
    #     plt.close()
    