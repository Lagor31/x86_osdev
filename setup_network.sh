sudo ip link add name br0 type bridge &
sudo ip link set dev br0 up &
sudo ip link set dev enp0s20f0u4 master br0

