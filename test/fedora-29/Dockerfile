# gcc-8.2
FROM fedora:29

RUN dnf -y update && \
	dnf -y install \
		make \
		libtool \
		autoconf \
		gcc \
		gcc-c++ \
		git \
		sqlite-devel \
		libsndfile-devel \
		gtk2-devel \
		libxml2-devel \
		libGLU-devel \
		fftw-devel

RUN echo "alias ll='ls -l'" >> /root/.bashrc
ENV DISPLAY :0
WORKDIR /root

ADD add_samples /root/add_samples
ADD compile /root/compile

CMD git clone https://github.com/ayyi/samplecat.git && cd samplecat && \
	git submodule update --init && git submodule foreach git pull origin master && \
	./autogen.sh && ./configure && make && \
	./src/samplecat --version
