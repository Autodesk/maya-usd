## Using AL's Docker scripts

### Prerequisites

AL_USDMaya's DockerFile needs to be based on a docker image that contains Maya. Build the wanted flavour of docker files from the docker-usd repository that contain the OS and maya version you would like to base your AL_USDMaya package on:

For example I want to base it on centos6 with maya2016 installed:
```
git clone https://github.com/AnimalLogic/docker-usd
cd docker-usd/linux
sudo ./build-centos6_maya2016.sh
```

### Build AL_USDMaya with Docker

To build AL_USDMaya's docker image:
```
git clone https://github.al.com.au/rnd/AL_USDMaya
cd AL_USDMaya
sudo ./build_docker_centos6.sh
```
Feel free to use any flavour of the build_docker_*.sh scripts. You just need to make sure you build the same flavour in the docker-usd repository.
