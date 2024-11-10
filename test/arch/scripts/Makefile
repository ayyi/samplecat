all:
	@echo branch=$(branch)
	@echo CI_COMMIT_BRANCH=$(CI_COMMIT_BRANCH)
	@git clone --branch "$(branch)" https://github.com/ayyi/samplecat.git
	@echo glib=`pkg-config glib-2.0 --modversion`
	@cd samplecat && \
		./autogen.sh && \
		./configure --enable-test && \
		cat lib/waveform/config.h | grep GLIB && \
		make && \
		./src/samplecat --version
	cd samplecat/test/unit && make check
	cd samplecat/lib/waveform && ./test/waveform
	./add_samples
