#ifndef CRYPTO_SERVICE
#define CRYPTO_SERVICE

#include <string>

static const size_t HASH_SIZE = 32;

/**
   * @brief The method converts hex (Hexadecimal) string to string
   *
   * @param[in] in the hex (Hexadecimal) string that should be converted to string
   * @param[in] data the converted hex to string
   * @return the converted string
   */
auto hexToCharArray(const std::string &in, unsigned char *data) -> int;

/**
   * @brief The method converts the string to hex (Hexadecimal) string
   *
   * @param[in] in the string that should be converted to hex (Hexadecimal) string
   * @param[in] length the length of string that should be converted to hex (Hexadecimal) string
   * @return the converted hex (Hexadecimal) string
   */
auto charArrayToHex(const unsigned char *data, unsigned length) -> std::string;

/**
 * @brief Generates the SHA256 hash of the provided data.
 *
 * @param message The data whose hash will be generated
 * @param message_len The length of the data
 * @param hash A pointer to the generated hash
 * @param hash_len The length of the generated hash
 * @return status code (0 success, 1 failure)
 */
auto hash_sha256(const unsigned char *data, size_t data_len,
                 unsigned char *hash, unsigned int *hash_len) -> int;

#endif
