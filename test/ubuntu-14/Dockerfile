FROM ubuntu:14.04
RUN apt-get -y update
RUN apt-get install -y openssh-client
RUN apt-get install -y \
	gcc make automake libtool pkg-config gdb \
	git \
	libsndfile1-dev \
	vim \
	libgtkglext1-dev \
	libxml2-dev \
	libsqlite3-dev \
	libavcodec-dev
RUN echo "alias ll='ls -l'" >> /root/.bashrc
ENV DISPLAY :0
WORKDIR /root
CMD mkdir samplecat && cd samplecat && \
	git clone https://github.com/ayyi/samplecat.git && cd samplecat && \
	git submodule update --init && git submodule foreach git pull origin master && \
	./autogen.sh && ./configure && make
