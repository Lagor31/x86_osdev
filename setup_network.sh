#!/bin/sh
sudo ip link add name br0 type bridge &
sudo ip link set dev br0 up &
sudo ip link set dev $1 master br0

