#ifndef AES_H
#define AES_H

#include "CH58x_common.h"

#define AESKEY	"ZTINFO----ZTINFO"
/**
 * ���� p: ���ĵ��ַ������顣
 * ���� plen: ���ĵĳ���,���ȱ���Ϊ16�ı�����
 * ���� key: ��Կ���ַ������顣
 */
void aes(char *p, int plen, char *key);

/**
 * ���� c: ���ĵ��ַ������顣
 * ���� clen: ���ĵĳ���,���ȱ���Ϊ16�ı�����
 * ���� key: ��Կ���ַ������顣
 */
void deAes(char *c, int clen, char *key);


void encryptData(char *encrypt,unsigned char *encryptlen,unsigned char *data,unsigned char len);
uint8_t dencryptData(char *dencrypt, unsigned char *dencryptlen,unsigned char *data, unsigned char len);

#endif

