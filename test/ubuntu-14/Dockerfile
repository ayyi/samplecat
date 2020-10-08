FROM ubuntu:14.04
RUN apt-get -y update && apt-get install -y \
	openssh-client \
	gcc make automake libtool pkg-config gdb \
	git \
	libsndfile1-dev \
	vim \
	libgtkglext1-dev \
	libxml2-dev \
	libyaml-dev \
	libsqlite3-dev \
	libavcodec-dev
RUN echo "alias ll='ls -l'" >> /root/.bashrc
ENV DISPLAY :0
WORKDIR /root
ADD Makefile /root/Makefile
CMD make
