sudo rmmod wait.ko
make clean
make
sudo insmod wait.ko
sudo mknod /dev/chardev c 511 0
