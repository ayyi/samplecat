FROM debian:buster
RUN apt-get -y update && apt-get install -y \
	openssh-client \
	gcc make automake libtool pkg-config gdb \
	git \
	vim \
	libsndfile1-dev \
	libgl1-mesa-dev \
	libxml2-dev
RUN apt-get install -y \
	gtk+2.0-dev

RUN echo "alias ll='ls -l'" >> /root/.bashrc
ENV DISPLAY :0
WORKDIR /root

ADD compile /root/compile

CMD git clone https://github.com/ayyi/samplecat.git && cd samplecat && \
	git submodule update --init && git submodule foreach git pull origin master && \
	./autogen.sh && ./configure && make
