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

def get_cdf(data):
    data.sort()
    result_dataset = [[],[]]
    for i in range(len(data)):
        result_dataset[0].append(data[i])
        result_dataset[1].append(float(i+1)/len(data))
    return result_dataset[0], result_dataset[1]

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
        
if __name__ == "__main__":
    file_names = ["no_tso","tso","tso_cedm","now"]
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
    print(qlen_data.keys())
    # slopes = [0]
    # weight = 0.12
    # for i in range(1,len(qlen_data["dequeue"])):
    #     slope_now = qlen_data["dequeue"][i] - qlen_data["dequeue"][i-1]
    #     slopes.append(slopes[-1]*(1-weight)+slope_now*weight)
    
    # ECN_marked_ts = []
    # ECN_marked_qlen = []
    # qlen_data["en_dequeue"] = []
    # ECN_threshold = 20000
    # for i in range(len(qlen_data["dequeue"])):
    #     j = i
    #     enqueue_slope = 0
    #     while j >= 0:
    #         if ts_data["dequeue"][j] < ts_data["enqueue"][i]:
    #             enqueue_slope = slopes[j]
    #             break
    #         j -= 1
    #     if j < 0:
    #         j = 0
    #     qlen_data["en_dequeue"].append(qlen_data["dequeue"][j])
    #     if enqueue_slope >= 0 and slopes[i] >= 0 and qlen_data["dequeue"][i] > ECN_threshold:
    #         ECN_marked_ts.append(ts_data["dequeue"][i])
    #         ECN_marked_qlen.append(qlen_data["dequeue"][i])

    for name in qlen_data:
        if "now" not in name:
            continue
        print(name)
        plt.figure(figsize=(8,4))
        plt.plot(ts_data[name]["dequeue"], qlen_data[name]["dequeue"],label = "dequeue", linestyle= '-',color="#4D72BE")
        plt.legend(loc="best",frameon=False,fontsize=24)
        plt.xlabel('time', fontsize=24)
        plt.ylabel('Qlen (B)', fontsize=24)
        #plt.ylim(0.8, 1.01)
        plt.xlim(0, 0.025)
        plt.tight_layout(pad=0)
        plt.show()
        #plt.savefig("./qlen.png",dpi=300)
        plt.clf()
        plt.close()
    for name in node_ts_throughput:
        if "now" not in name:
            continue
        print(name)
        plt.figure(figsize=(8,4))
        plt.plot(node_ts_throughput[name], node_throughput[name], linestyle= '-',color="#4D72BE")
        plt.legend(loc="best",frameon=False,fontsize=24)
        plt.xlim(0, 0.025)
        plt.xlabel('time', fontsize=24)
        plt.ylabel('Mbps', fontsize=24)
        plt.tight_layout(pad=0)
        plt.show()
        plt.clf()
        plt.close()
    
    #plt.plot(ts_data["enqueue"], qlen_data["enqueue"],label = "enqueue", linestyle= '-',color="#5C803F") #linewidth=4, marker='o', markersize=10
    #plt.plot( qlen_data["enqueue"],label = "mark", linestyle= '-',color="r",marker='x')
    #plt.scatter(ECN_marked_ts, ECN_marked_qlen, color="brown", marker='x')
    #plt.plot(ts_data["dequeue"], qlen_data["en_dequeue"],label = "en_dequeue", linestyle= '-',color="black")
    #plt.plot(ts_data[(0,1)]["dequeue"], qlen_data[(0,1)]["dequeue"],label = "dequeue", linestyle= '-',color="#4D72BE")

    #plt.plot([0,30],[0.1,0.1], "--",linewidth=2,color="k")
    #plt.plot([ts_data["dequeue"][0],ts_data["dequeue"][-1]],[ECN_threshold, ECN_threshold], "--",linewidth=1,color="r")
    #plt.text(13, 2.5, "BWO 0.1%", size = 24, style="italic",color = "k", weight = "light")
    ###LEGEND
    #plt.legend(loc="best",frameon=False,fontsize=24)
    #plt.xlabel('time', fontsize=24)
    #plt.ylabel('Qlen (B)', fontsize=24)
    #plt.ylim(0.8, 1.01)
    #plt.xlim(0, 0.025)
    #plt.xticks([1, 5, 10, 15,20, 25,30], [1, 5, 10, 15,20, 25,30],fontsize=22)  # Set text labels and properties.
    #plt.xticks(fontsize=22) 
    #plt.yticks([0.8, 0.84, 0.88, 0.92, 0.96, 1.0], [0.8, 0.84, 0.88, 0.92, 0.96, 1.0],fontsize=22) 
    #plt.title("Elephant-Mice Flow")
    #plt.tight_layout(pad=0)
    #plt.show()
    #plt.savefig("./qlen.png",dpi=300)
    #plt.clf()

    ####  Plot CDF  ####
    plt.figure()
    xxx = 0
    yyy = 0
    for name in qlen_data:
        print(name)
        for i in range(len(qlen_data[name]["dequeue"])):
            if ts_data[name]["dequeue"][i] > 0.01 and xxx == 0:
                print(i)
                xxx = 1
            elif ts_data[name]["dequeue"][i] > 0.025 and yyy == 0:
                print(i)
                yyy = 1
        data_x, data_y = get_cdf(qlen_data[name]["dequeue"][7900:20400])
        plt.plot(data_x, data_y, label=name)
    plt.legend(loc="best",frameon=False,fontsize=24)
    plt.xlabel('qlen (B)', fontsize=24)
    plt.ylabel('CDF', fontsize=24)
    #plt.ylim(0.8, 1.01)
    #plt.xlim(0, 0.025)
    plt.tight_layout(pad=0)
    plt.show()
    #plt.savefig("./qlen.png",dpi=300)
    plt.clf()
    plt.close()

    plt.figure()
    xxx = 0
    yyy = 0
    for name in node_ts_throughput:
        print(name)
        for i in range(len(node_ts_throughput[name])):
            if node_ts_throughput[name][i] > 0.01 and xxx == 0:
                print(i)
                xxx = 1
            elif node_ts_throughput[name][i] > 0.025 and yyy == 0:
                print(i)
                yyy = 1
        data_x, data_y = get_cdf(node_throughput[name][881:2233])
        plt.plot(data_x, data_y, label=name)
    plt.legend(loc="best",frameon=False,fontsize=24)
    plt.xlabel('Mbps', fontsize=24)
    plt.ylabel('CDF', fontsize=24)
    #plt.ylim(0.8, 1.01)
    #plt.xlim(0, 0.025)
    plt.tight_layout(pad=0)
    plt.show()
    #plt.savefig("./qlen.png",dpi=300)
    plt.clf()
    plt.close()