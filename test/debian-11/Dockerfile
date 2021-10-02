FROM debian:bullseye
RUN apt-get -y update && apt-get install -y \
	openssh-client \
	gcc make automake libtool pkg-config gdb \
	git \
	libsqlite3-dev \
	libsndfile1-dev \
	libgl1-mesa-dev \
	libxml2-dev \
	libyaml-dev \
	libgraphene-1.0-dev \
	gtk+2.0-dev

RUN echo "alias ll='ls -l'" >> /root/.bashrc
ENV DISPLAY :0
WORKDIR /root

ADD scripts/Makefile /root/Makefile

CMD make
