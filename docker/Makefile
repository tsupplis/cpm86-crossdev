
all: images

images: cpm86-latest

cpm86-latest:
	docker build \
        --build-arg USER_ID=`id -u` \
        --build-arg USER=$(USER) \
        --build-arg HOME=$(HOME) --rm=true -t cpm86:latest -f Dockerfile .

clean: rmi

rmi:
	-docker rmi cpm86:latest 
