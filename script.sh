#!/bin/bash
set -ei

mkdir -P pics

while [[ 1 ]]; do
    fswebcam -d /dev/video1 -r 1280x720 ./pics/$(date +"%Y-%m-%d-%H_%M").jpg
    sleep 60
done
