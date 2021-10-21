
all: images

images: cpm86-latest

cpm86-latest:
	docker build \
        --rm=true -t cpm86:latest -f Dockerfile . 2> build.log >&2

clean: 
	-docker rmi cpm86:latest
	-docker system prune -f
