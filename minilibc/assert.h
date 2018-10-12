#ifndef MINILIBC_ASSERT_H
#define MINILIBC_ASSERT_H

int assertion_failed(const char *i, const long);  /* fictional return type to satisfy expression operators */
#define assert(cnd) ((cnd) || assertion_failed(__FILE__, __LINE__))

#endif
