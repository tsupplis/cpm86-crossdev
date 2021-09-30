
all: images

images: cpm86-latest

cpm86-latest:
	docker build \
        --rm=true -t cpm86:latest -f Dockerfile .

clean: rmi

rmi:
	-docker rmi cpm86:latest 
