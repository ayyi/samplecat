name = $(shell cat name)

Dockerfile: Dockerfile.in dependencies
	cpp -P -traditional -o Dockerfile Dockerfile.in

build: Dockerfile dependencies add_samples
	docker build -t $(name) .

.PHONY: build
