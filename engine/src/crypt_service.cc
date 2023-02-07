#include "blockchain/crypt_service.h"
#include "trustdble_utils/encoding_helpers.h"
#include <iomanip>
#include <iostream>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>
#include <string>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <fstream>

void handleErrors(void) {
  ERR_print_errors_fp(stderr);
  abort();
}

auto hash_sha256(const unsigned char *data, size_t data_len,
                 unsigned char *hash, unsigned int *hash_len) -> int {
  EVP_MD_CTX *mdctx;

  if ((mdctx = EVP_MD_CTX_new()) == NULL)
    handleErrors();

  if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL))
    handleErrors();

  if (1 != EVP_DigestUpdate(mdctx, data, data_len))
    handleErrors();

  if (1 != EVP_DigestFinal_ex(mdctx, hash, hash_len))
    handleErrors();

  EVP_MD_CTX_free(mdctx);

  return 0;
}
