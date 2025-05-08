#!/bin/sh

docker build --no-cache -t satcomp-prs-sbva:common ../../ --file common/Dockerfile
docker build --no-cache -t satcomp-prs-sbva:leader ../../ --file leader/Dockerfile