# Introduction
This is a simple implementation of TSO processing in TCP layer.
The implementation follows GNU/Linux 2.6.38.3.

Note: we only implement colascing data in TCP layer.
Sending a larger virtual packet into NIC layer and segmentation offload is our future work.

# Example
In directory `example`, run simulation with script `run_baseline.sh`


# How to use
Use the TCP Agent `Agent/TCP/FullTcp`.

Enable TSO:
``` tcl
Agent/TCP/FullTcp set tso_enable_ true
```

Change TSO settings:
```tcl
# maximum TSO size in Bytes
Agent/TCP/FullTcp set max_tso_size_ 65536
# the same meaning as the sysctl parameter net.ipv4.tcp_tso_win_divisor in Linux
Agent/TCP/FullTcp set tcp_tso_win_divisor_ 3
```


# References
Danfeng Shan, Fengyuan Ren, "Improving ECN Marking Scheme with Micro-burst Traffic in Data Center Networks", IEEE INFOCOM, Atlanta, GA, May, 2017.

Bibtex:

	@inproceedings {infocom17cedm,
		author = {Danfeng Shan and Fengyuan Ren},
    	title = {Improving ECN Marking Scheme with Micro-burst Traffic in Data Center Networks},
    	booktitle = {IEEE INFOCOM},
    	year = {2017},
	}
