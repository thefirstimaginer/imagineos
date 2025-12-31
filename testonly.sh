cd ./OS-SOURCE/
make build-x86_64
cd ..
qemu-system-amd64 -cdrom ./OS-SOURCE/distro/x86_64/ImagineOS-READY.iso
cd ./OS-SOURCE/
make clean
cd ..
