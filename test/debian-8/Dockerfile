FROM debian:jessie
RUN apt-get -y update && apt-get install -y \
	openssh-client \
	gcc make automake libtool pkg-config \
	git \
	vim \
	libsndfile1-dev \
	libgl1-mesa-dev \
	libxml2-dev \
	gtk+2.0-dev
RUN echo "alias ll='ls -l'" >> /root/.bashrc
#CMD scp -p tim@10.0.0.160:docs/devel/samplecat-git/samplecat*gz .
#CMD curl http://www.orford.org/assets/samplecat-0.2.4.tar.gz > samplecat-0.2.4.tar.gz && tar xf samplecat-0.2.4.tar.gz && cd samplecat-0.2.4 && ./configure && ./make
ENV DISPLAY :0
WORKDIR /root
ADD Makefile /root/Makefile
CMD make
