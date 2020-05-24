all:
	git clone --branch "$(CI_COMMIT_BRANCH)" https://github.com/ayyi/samplecat.git
	cd samplecat && \
		git submodule update --init && \
		./autogen.sh && \
		./configure && \
		make
