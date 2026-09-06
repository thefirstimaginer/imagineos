# Testes e criterio de estabilidade

## Build

```sh
make clean && make iso
```

O build deve terminar sem erro de compilacao, link ou geracao da ISO.

## Boot repetido

```sh
for attempt in 1 2 3 4 5; do
    timeout 8s qemu-system-x86_64 \
      -no-reboot -display none -serial stdio \
      -cdrom distro/imos.iso
    echo "status=$?"
done
```

Um status `124` significa que o processo foi encerrado pelo timeout. Qualquer status diferente deve ser investigado.

## Diagnostico de reset

```sh
qemu-system-x86_64 -no-reboot -display none \
  -serial stdio -d cpu_reset,int \
  -D /tmp/imagineos-qemu.log \
  -cdrom distro/imos.iso
```

Registrar sempre:

- ultima mensagem do kernel;
- RIP;
- RSP;
- CR2;
- CR3;
- vetor da excecao;
- se ocorreu triple fault.

## Teste manual do shell

Depois que o prompt aparecer:

```text
help
test
echo hello
clear
proc-test
```

O resultado esperado de `proc-test` e:

```text
[OK] child process started
[OK] fork/waitpid/exit test passed
```

No estado atual, qualquer reinicio durante esse comando e uma falha conhecida, nao um resultado valido.

## Regra de validacao

Nao considerar uma funcionalidade estavel por uma unica execucao bem-sucedida. Para processos e memoria, repetir o teste varias vezes e coletar logs de QEMU.
