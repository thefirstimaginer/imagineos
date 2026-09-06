# Build e execucao

## Dependencias

O ambiente precisa fornecer:

- GCC com suporte x86_64;
- NASM;
- GNU ld;
- GRUB `grub-mkrescue`;
- QEMU x86_64.

## Compilar a ISO

Na raiz do repositorio:

```sh
make clean
make iso
```

A ISO sera gerada em `distro/imos.iso`.

## Executar com janela QEMU

```sh
make run
```

Ou:

```sh
make qemu
```

## Executar sem janela

Para verificar se a VM permanece viva por um periodo curto:

```sh
timeout 8s qemu-system-x86_64 \
  -no-reboot -display none -serial stdio \
  -cdrom distro/imos.iso
```

O timeout `124` significa que o processo foi encerrado pelo `timeout`; nesse teste isso indica que a VM permaneceu executando. Status `0` inesperado pode indicar desligamento, reset ou falha no guest e deve ser investigado com logs.

## Diagnostico de faults

```sh
qemu-system-x86_64 -no-reboot -display none \
  -serial stdio -d cpu_reset,int \
  -D /tmp/imagineos-qemu.log \
  -cdrom distro/imos.iso
```

Procure por:

- `Triple fault`;
- `check_exception`;
- `RIP=`;
- `CR2=`;
- `CR3=`.

## Modulos

O `Makefile` constroi e copia para a ISO:

- `.build/imos.elf`;
- `.build/init.elf`;
- `.build/shell.elf`;
- `.build/clear.elf`.

Os nomes dos modulos precisam coincidir com os nomes procurados no loader e no script de init.
