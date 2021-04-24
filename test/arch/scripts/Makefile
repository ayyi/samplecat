all:
	@echo branch=$(branch)
	@git clone --branch "$(branch)" https://github.com/ayyi/samplecat.git
	@echo glib=`pkg-config glib-2.0 --modversion`
	@cd samplecat && \
		./autogen.sh && \
		./configure --enable-test && \
		cat lib/waveform/config.h | grep GLIB && \
		make && \
		./src/samplecat --version
	cd samplecat/lib/waveform && ./test/waveform_test
	./add_samples
