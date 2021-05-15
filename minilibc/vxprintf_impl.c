/* This is a macro-like file intended to be included multiple times with care.
 * Please don't include this casually.
 *
 * In practice, TL only needs s, p, and ld, which are exclusively implemented.
 *
 * Args:
 * VXP_NAME: name of the declared function
 * VXP_ARGS: tokens before format string in arg list, with trailing comma if needed
 * VXP_PUTC: called to write a character
 * VXP_DONE: replaced before returning
 */

int VXP_NAME(VXP_ARGS const char *fmt, va_list ap) {
	int cnt = 0;
	long i;
	union {
		void *vptr;
		char *cptr;
		long lint;
		unsigned long ulint;
	} temp;
	while(*fmt) {
		if(*fmt == '%') {
			fmt++;
			switch(*fmt) {
				case '%':
					VXP_PUTC('%');
					cnt++;
					fmt++;
					break;

				case 's':
					temp.cptr = va_arg(ap, char *);
					while(*temp.cptr) {
						VXP_PUTC(*temp.cptr);
						temp.cptr++;
						cnt++;
					}
					fmt++;
					break;

				case 'p':
					temp.vptr = va_arg(ap, void *);
					for(i = 8*sizeof(void *)-4; i >= 0; i -= 4) {
						VXP_PUTC("0123456789ABCDEF"[(temp.ulint >> i) & 0xF]);
						cnt++;
					}
					fmt++;
					break;

				case 'd':
					temp.lint = (long)va_arg(ap, int);
					goto decimal;

				case 'l':  /* XXX d */
					temp.lint = va_arg(ap, long);
decimal:
					i = 1;
					if(temp.lint < 0) {
						VXP_PUTC('-');
						cnt++;
						i = -1;
					}
					while(temp.lint / i >= 10) i *= 10;
					do {
						VXP_PUTC("0123456789"[temp.lint / i % 10]);
						cnt++;
						i /= 10;
					} while(i > 1 || i < -1);
					fmt += 2;  /* skip over presupposed d*/
					break;

				default:
					VXP_PUTC('%');
					cnt++;
					VXP_PUTC(*fmt);
					cnt++;
					fmt++;
			}
		} else {
			VXP_PUTC(*fmt);  /* carefully avoid side effects */
			cnt++;
			fmt++;
		}
	}
	VXP_DONE;
	return cnt;
}

/* Clean up the namespace */
#undef VXP_NAME
#undef VXP_ARGS
#undef VXP_PUTC
#undef VXP_DONE
