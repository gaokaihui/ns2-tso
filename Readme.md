# Introduction
This is a simple implementation of TSO processing in TCP layer.
The implementation follows GNU/Linux 2.6.38.3.

Note: we only implement colascing data in TCP layer.
Sending a larger virtual packet into NIC layer and segmentation offload is our future work.

# References
Danfeng Shan, Fengyuan Ren, "Improving ECN Marking Scheme with Micro-burst Traffic in Data Center Networks", IEEE INFOCOM, Atlanta, GA, May, 2017.

Bibtex:

	@inproceedings {infocom17cedm,
		author = {Danfeng Shan and Fengyuan Ren},
    	title = {Improving ECN Marking Scheme with Micro-burst Traffic in Data Center Networks},
    	booktitle = {IEEE INFOCOM},
    	year = {2017},
	}
