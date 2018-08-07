echo "Build AL_USDMaya"
docker build -t "usd-docker/alusdmaya:0.20.2-centos6" -f docker/Dockerfile_centos6 .
docker tag -f usd-docker/alusdmaya:0.20.2-centos6 usd-docker/alusdmaya:0.20.2-centos6
docker tag -f usd-docker/alusdmaya:0.20.2-centos6 usd-docker/alusdmaya:latest-centos6
