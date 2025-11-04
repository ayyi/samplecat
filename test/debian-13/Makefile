BRANCH ?= master

all:
	git clone --branch "$(BRANCH)" https://github.com/ayyi/samplecat.git
	cd samplecat && \
		./autogen.sh && \
		./configure && \
		make -j5
