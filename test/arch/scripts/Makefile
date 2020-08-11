all:
	@echo branch=$(branch)
	@git clone --branch "$(branch)" https://github.com/ayyi/samplecat.git
	@cd samplecat && \
		git submodule update --init && \
		./autogen.sh && \
		./configure --enable-test && \
		make && \
		./src/samplecat --version
	cd samplecat/lib/waveform && ./test/waveform_test
	./add_samples
