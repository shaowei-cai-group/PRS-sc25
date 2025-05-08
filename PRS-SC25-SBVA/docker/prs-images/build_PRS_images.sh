#!/bin/sh

docker build -t satcomp-prs-sbva:common ../../ --file common/Dockerfile
docker build -t satcomp-prs-sbva:leader ../../ --file leader/Dockerfile

# docker build -t satcomp-prs-sbva:common ../ --file common/Dockerfile
# docker build -t satcomp-prs-sbva:leader ../ --file prs-sc25-sbva/Dockerfile