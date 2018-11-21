#ifndef AES_128_CFB_H
#define AES_128_CFB_H

#define AES_BLOCK_SIZE 16

void keyExpansion(unsigned char* key, unsigned char w[][4][4]);
unsigned char* cipher(unsigned char* input, unsigned char* output, unsigned char w[][4][4]);
unsigned char* invCipher(unsigned char* input, unsigned char* output, unsigned char w[][4][4]);
unsigned char FFmul(unsigned char a, unsigned char b);
void subBytes(unsigned char state[][4]);
void shiftRows(unsigned char state[][4]);
void mixColumns(unsigned char state[][4]);
void invSubBytes(unsigned char state[][4]);
void invShiftRows(unsigned char state[][4]);
void invMixColumns(unsigned char state[][4]);
void addRoundKey(unsigned char state[][4], unsigned char k[][4]);
int AES_128_CFB_Encrypt(unsigned char* key, unsigned char* iv,
						unsigned char* indata, int inlen, unsigned char* outData);
int AES_128_CFB_Decrypt(unsigned char* key, unsigned char* iv,
						unsigned char* inData, int inLen, unsigned char* outData);

#endif // AES_128_CFB_H
