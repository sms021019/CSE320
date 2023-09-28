/*
 * Error handling routines
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

int errors;
int warnings;
int dbflag = 1;

void fatal(char *fmt, ...)

{
        va_list argp;
        if(fmt == NULL){
                return;
        }
        fprintf(stderr, "\nFatal error: ");
        vfprintf(stderr, fmt, argp);
        fprintf(stderr, "\n");
        exit(1);
}

void error(char *fmt, ...)
{
        va_list argp;
        if(fmt == NULL){
                return;
        }
        fprintf(stderr, "\nError: ");
        vfprintf(stderr, fmt, argp);
        fprintf(stderr, "\n");
        errors++;
}

void warning(char *fmt, ...)
{
        va_list argp;
        if(fmt == NULL){
                return;
        }
        fprintf(stderr, "\nWarning: ");
        vfprintf(stderr, fmt, argp);
        fprintf(stderr, "\n");
        warnings++;
}

void debug(char *fmt, ...)
{
        if(!dbflag) return;
        va_list argp;
        if(fmt == NULL) return;
        fprintf(stderr, "\nDebug: ");
        fprintf(stderr, fmt, argp);
        fprintf(stderr, "\n");
}
