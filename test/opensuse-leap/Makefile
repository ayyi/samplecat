Dockerfile: Dockerfile.in dependencies
	cpp -P -traditional -o Dockerfile Dockerfile.in

build: Dockerfile dependencies add_samples
	docker build -t samplecat-opensuse-leap .

.PHONY: build
