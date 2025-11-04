FROM registry.gitlab.com/ayyi.org/docker/debian-13
RUN apt-get -y update && apt-get install -y \
	g++ \
	libsqlite3-dev \
	libsndfile1-dev \
	libxml2-dev \
	libgtk-4-dev

RUN echo "alias ll='ls -l'" >> /root/.bashrc
ENV DISPLAY :0
WORKDIR /root

ADD Makefile /root/Makefile

CMD make
