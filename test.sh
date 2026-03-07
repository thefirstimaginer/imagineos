cd ./main/
make build-x86_64
cd ..
qemu-system-amd64 -cdrom ./main/distro/x86_64/main.iso
cd ./main/
make clean
cd ..
