FROM debian:jessie
RUN apt-get -y update
RUN apt-get install -y openssh-client
RUN apt-get install -y \
	gcc make automake libtool pkg-config \
	git \
	libsndfile1-dev \
	vim \
	libgtkglext1-dev \
	libxml2-dev
RUN echo "alias ll='ls -l'" >> /root/.bashrc
#CMD scp -p tim@10.0.0.160:docs/devel/samplecat-git/samplecat*gz .
#CMD curl http://www.orford.org/assets/samplecat-0.2.4.tar.gz > samplecat-0.2.4.tar.gz && tar xf samplecat-0.2.4.tar.gz && cd samplecat-0.2.4 && ./configure && ./make
#ADD build.sh
#CMD ./build.sh
ENV DISPLAY :0
WORKDIR /root
CMD mkdir samplecat && cd samplecat && \
	git clone https://github.com/ayyi/samplecat.git && cd samplecat && \
	git submodule update --init && git submodule foreach git pull origin master && \
	./autogen.sh && ./configure && make
