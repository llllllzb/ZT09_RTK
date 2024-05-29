/*
 * base64.h
 *
 *  Created on: Jul 24, 2023
 *      Author: lvvv
 */

#ifndef TASK_INC_BASE64_H_
#define TASK_INC_BASE64_H_

#include "config.h"

#define BASE64_PAD '='
#define BASE64DE_FIRST '+'
#define BASE64DE_LAST 'z'

#define BASE64_ENCODE_OUT_SIZE(s) ((unsigned int)((((s) + 2) / 3) * 4 + 1))
#define BASE64_DECODE_OUT_SIZE(s) ((unsigned int)(((s) / 4) * 3))


/*
 * out is null-terminated encode string.
 * return values is out length, exclusive terminating `\0'
 */
unsigned int base64_encode(const unsigned char *in, unsigned int inlen, char *out);

/*
 * return values is out length
 */
unsigned int base64_decode(const char *in, unsigned int inlen, unsigned char *out);


#endif /* TASK_INC_BASE64_H_ */
