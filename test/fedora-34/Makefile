Dockerfile: Dockerfile.in dependencies
	cpp -P -traditional -o Dockerfile Dockerfile.in

build: Dockerfile dependencies scripts/add_samples
	docker build -t samplecat-fedora-34 .

.PHONY: build
