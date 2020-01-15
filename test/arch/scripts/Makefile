all:
	@git clone https://github.com/ayyi/samplecat.git
	@cd samplecat && \
		git submodule update --init && git submodule foreach git pull origin master && \
		./autogen.sh && \
		./configure --enable-test && \
		make && \
		./src/samplecat --version
	cd samplecat/lib/waveform && ./test/waveform_test
	./add_samples
