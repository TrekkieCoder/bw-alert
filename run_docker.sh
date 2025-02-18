#!/bin/bash
docker run --rm --network host --privileged --restart unless-stopped ghcr.io/loxilb-io/bw-alert:latest eth0 1 50 100 "172.17.0.3,192.168.1.100" "10.10.10.1"

