# gcc 7.3
FROM ubuntu:18.04
RUN apt-get -y update
RUN apt-get install -y openssh-client

# - includes support for running Opengl applications
# - ffmpeg included for creating test data (adds ~100MB to image size)
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
	gcc make automake libtool pkg-config gdb \
	git \
	libsndfile1-dev \
	ffmpeg \
	vim \
	libgtkglext1-dev \
	libxml2-dev \
	libsqlite3-dev \
	mesa-utils \
	xserver-xorg-video-all
RUN echo "alias ll='ls -l'" >> /root/.bashrc
ENV DISPLAY :0
WORKDIR /root
ADD add_samples /root/add_samples
ADD compile /root/compile
CMD git clone https://github.com/ayyi/samplecat.git && cd samplecat && \
	git submodule update --init && git submodule foreach git pull origin master && \
	./autogen.sh && ./configure && make
