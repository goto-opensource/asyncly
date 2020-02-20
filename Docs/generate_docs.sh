#!/usr/bin/env bash

docker build . -t executordocs
docker run -v `pwd`/..:/build --rm executordocs
docker rmi executordocs
