#ifndef IMAGINEOS_KASSERT_H
#define IMAGINEOS_KASSERT_H

void kassert_fail(const char *expression, const char *file, int line);

#define kassert(expression) \
    ((expression) ? (void)0 : kassert_fail(#expression, __FILE__, __LINE__))

#endif