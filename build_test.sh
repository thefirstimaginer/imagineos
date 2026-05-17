echo "[Iniciando build do ImagineOS, Aguarde]"
cd ./main/
make
cd ..
qemu-system-amd64 -cdrom ./main/distro/x86_64/main.iso
