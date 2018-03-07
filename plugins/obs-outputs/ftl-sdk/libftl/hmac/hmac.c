#include <string.h>

#include "hmac.h"
#include "sha2.h"
#include <stdio.h>

#define O_KEY_PAD 0x5C
#define I_KEY_PAD 0x36

const unsigned char hex_digits[] = "0123456789abcdef";

int hmacsha512(const char * rawKey, const unsigned char * message, const int messageLength, char * result) {
  Sha512Context ctx;
  SHA512_HASH computedHash;
  size_t keyLen = strlen(rawKey);

  const char * key;
  if (keyLen > SHA512_BLOCK_SIZE) {
    Sha512Initialise(&ctx);
    Sha512Update(&ctx, (void*)rawKey, (uint32_t)keyLen);
    Sha512Finalise(&ctx, &computedHash);

    key = (const char *)computedHash.bytes;
    keyLen = SHA512_HASH_SIZE;
  } else {
    key = rawKey;
  }

  unsigned char iKeyPad[SHA512_BLOCK_SIZE];
  unsigned char oKeyPad[SHA512_BLOCK_SIZE];

  memset(oKeyPad, O_KEY_PAD, SHA512_BLOCK_SIZE);
  memset(iKeyPad, I_KEY_PAD, SHA512_BLOCK_SIZE);

  size_t i = 0;
  for(i = 0; i < keyLen; i++) {
    oKeyPad[i] ^= key[i];
    iKeyPad[i] ^= key[i];
  }

  Sha512Initialise(&ctx);
  Sha512Update(&ctx, (void*)iKeyPad, SHA512_BLOCK_SIZE);
  Sha512Update(&ctx, (void*)message, messageLength);

  Sha512Finalise(&ctx, &computedHash);

  Sha512Initialise(&ctx);
  Sha512Update(&ctx, (void*)oKeyPad, SHA512_BLOCK_SIZE);
  Sha512Update(&ctx, (void*)computedHash.bytes, SHA512_HASH_SIZE);

  Sha512Finalise(&ctx, &computedHash);

  for(i = 0; i < SHA512_HASH_SIZE; i++) {
    result[i * 2] = hex_digits[computedHash.bytes[i] >> 4];
    result[(i * 2) + 1] = hex_digits[computedHash.bytes[i] & 0x0F];
  }

  result[SHA512_HEX_STRING_HASH_SIZE - 1] = 0;

  memset(computedHash.bytes, 0, sizeof(computedHash.bytes));

  return SHA512_HEX_STRING_HASH_SIZE;
}
