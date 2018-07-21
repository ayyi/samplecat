#!/bin/sh
cd /root && mkdir samplecat && cd samplecat && \
	git clone https://github.com/ayyi/samplecat.git && cd samplecat && \
	git submodule init && git submodule update && git submodule foreach git pull origin master && \
	./autogen.sh && ./configure && make
