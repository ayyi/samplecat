# No package for libgraphene

FROM ubuntu:16.04

RUN apt-get update && apt-get install -y \
	openssh-client \
	gcc make automake libtool pkg-config \
	gdb \
	git \
	vim \
	libsndfile1-dev \
	libgtkglext1-dev \
	libxml2-dev \
	libyaml-dev \
	libsqlite3-dev

RUN echo "alias ll='ls -l'" >> /root/.bashrc
ENV DISPLAY :0
WORKDIR /root
ADD compile /root/compile

CMD git clone https://github.com/ayyi/samplecat.git && cd samplecat && \
	git submodule update --init && git submodule foreach git pull origin master && \
	./autogen.sh && ./configure && make
