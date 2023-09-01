/*
 * 
*/

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include "c-types.h"

#define LOG_FILE_MAX		0x10000
#include "panic.h"

char __panic_logfile[PATH_MAX] = "";

void setlogfile(const char *src)
{ strcpy(__panic_logfile, src); }

void logpf(const char *fmt, ...) {
	FILE *fp;
	va_list ap;
	struct stat st;

	if ( __panic_logfile[0] == '\0' ) strcpy(__panic_logfile, "/tmp/xxx.log");
	va_start(ap, fmt);
	if ( stat(__panic_logfile, &st) == 0 ) {
		if ( st.st_size >= LOG_FILE_MAX ) {
			if ( (fp = fopen(__panic_logfile, "r")) != NULL ) {
				char *buf = (char *) malloc(st.st_size + 1);
				if ( buf == NULL ) PANIC("Out of memory!");
				char *p = buf;
				int_t ln = 0, remove_lines = st.st_size >> 8;

				fread(buf, st.st_size, 1, fp);
				fclose(fp);
				
				buf[st.st_size] = '\0';
				while ( (p = strrchr(p+1, '\n')) != NULL ) {
					ln ++;
					if ( ln == remove_lines )
						break;
					}
				if ( p ) *p = '\0';

				if ( (fp = fopen(__panic_logfile, "w")) != NULL ) {
					fwrite(buf, strlen(buf), 1, fp);
					fclose(fp);
					}
				free(buf);
				}
			}
		}
	if ( (fp = fopen(__panic_logfile, "a")) != NULL ) {
		vfprintf(fp, fmt, ap);
		fclose(fp);
		}
	va_end(ap);
	}

