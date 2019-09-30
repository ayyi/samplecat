Dockerfile: Dockerfile.in dependencies
	cpp -P -traditional -o Dockerfile Dockerfile.in

build: Dockerfile dependencies add_samples
	docker build -t samplecat-ubuntu-18 .

.PHONY: build
