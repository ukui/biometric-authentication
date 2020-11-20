/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * Author: Droiing <jianglinxuan@kylinos.cn>
 *         chenziyi <chenziyi@kylinos.cn>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
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
