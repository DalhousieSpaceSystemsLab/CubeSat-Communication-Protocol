/**
 * @file panic.h
 * @author Alex Amellal (loris@alexamellal.com)
 * @brief Implementation of flow control via panics in C
 * @version 0.1
 * @date 2022-02-25
 * 
 * @copyright Dalhousie Space Systems Lab (c) 2022
 * 
 */

#ifndef __PANIC_H__
#define __PANIC_H__

extern int panic_errno;

#define ENABLE_PANIC static int panic_errno = 0
#define panic(val) panic_errno = val; goto cleanup

#endif 