# Debian stretch
# Security updates have been discontinued as of July 6th, 2020
# Stretch benefits from Long Term Support (LTS) until the end of June 2022
#
# There is no libgraphene package for Stretch
FROM debian:stretch
RUN apt-get -y update
RUN apt-get install -y openssh-client
RUN apt-get install -y \
	gcc make automake libtool pkg-config gdb \
	git \
	libsndfile1-dev \
	vim \
	libsqlite3-dev \
	libgtkglext1-dev \
	libxml2-dev
RUN echo "alias ll='ls -l'" >> /root/.bashrc
ENV DISPLAY :0
WORKDIR /root
CMD git clone https://github.com/ayyi/samplecat.git && cd samplecat && \
	git submodule update --init && git submodule foreach git pull origin master && \
	./autogen.sh && ./configure && make
