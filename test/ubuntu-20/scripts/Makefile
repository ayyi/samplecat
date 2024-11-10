all:
	git clone --branch "$(branch)" https://github.com/ayyi/samplecat.git
	cd samplecat && \
		./autogen.sh && \
		./configure --enable-test && \
		make && \
		./src/samplecat --version
	cd samplecat/lib/waveform && ./test/waveform
	cd samplecat/test/unit && make check
	./add_samples
