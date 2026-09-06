#ifndef IMAGINEOS_KERNEL_INPUT_H
#define IMAGINEOS_KERNEL_INPUT_H

void input_init(void);
long input_read(char *buffer, unsigned long count);
long input_read_nonblock(char *buffer, unsigned long count);

#endif
