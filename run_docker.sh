#!/bin/bash
docker run --rm --network host --privileged ghcr.io/loxilb-io/bw-alert:latest eth0 1 50 "172.17.0.3,192.168.1.100" "10.10.10.1"

