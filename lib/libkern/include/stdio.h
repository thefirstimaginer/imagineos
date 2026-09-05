#pragma once

#ifndef IMAGINEOS_LIBKERN_STDIO_H
#define IMAGINEOS_LIBKERN_STDIO_H

#include "kprintf.h"

#define printf(...) kputs(__VA_ARGS__)

#endif