/**
 * debug.h
 * Autor: Ondřej Fojt (fojt.ondrej@seznam.cz)
 * Projekt: Home Thermostat
 */
/**
 * Description:
 * 
*/
#ifndef DEBUG_H
#define DEBUG_H

#if DEBUG == 1
#include <stdio.h>
#define debug(msg, ...) fprintf(stderr,  __FILE__ ":%u: " msg "\n", __LINE__, __VA_ARGS__ ) 
#else 
#define debug(...)
#endif

#endif