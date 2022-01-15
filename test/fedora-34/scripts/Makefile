all:
	git clone --branch "$(branch)" https://github.com/ayyi/samplecat.git
	cd samplecat && \
		./autogen.sh && \
		./configure && \
		make && \
		./src/samplecat --version
