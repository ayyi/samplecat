all:
	git clone --branch "$(BRANCH)" https://github.com/ayyi/samplecat.git
	cd samplecat && \
		git submodule update --init && \
		./autogen.sh && \
		./configure && \
		make
