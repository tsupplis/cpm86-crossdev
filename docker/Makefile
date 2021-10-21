
all: images

images: cpm86-latest

cpm86-latest:
	docker build \
        --rm=true -t cpm86:latest -f Dockerfile .

clean: 
	-docker rmi cpm86:latest
	-docker system prune -f
