all:
	git clone --branch "$(branch)" https://github.com/ayyi/samplecat.git
	cd samplecat && \
		./autogen.sh && \
		./configure --enable-test && \
		make && \
		./src/samplecat --version
	cd samplecat/lib/waveform && ./test/waveform_test
	cd samplecat/test/unit && ./unit_test
	./add_samples
