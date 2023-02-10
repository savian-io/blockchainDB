#ifndef ENCODING_HELPERS_H
#define ENCODING_HELPERS_H

#include <iomanip>
#include <iostream>
#include <string>


static const size_t HASH_SIZE = 32;

#define ENCODED_BYTE_SIZE 16

// keys and values of smart contrat are 32 byte and represented as hex
// strings with 64 chars. Each byte as 2 chars
#define VALUE_SIZE 64


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


/**
 * @brief Helper-Method to convert an integer into its hex-representation as
 * string.
 *
 * @param num The integer to be converted.
 *
 * @param size The total size of the string output. Default=VALUE_SIZE of
 * ethereum (64).
 *
 * @return Hex-encoded string of lenght size. It will be filled up with
 * leading zeros.
 */
auto int_to_hex(size_t num, int size = VALUE_SIZE) -> std::string;

/**
 * @brief Helper-Method to convert a hex-coded string into an integer.
 *
 * @param hex The hex-encoded string to be converted.
 *
 * @return Integer of the hex-coded string.
 */
auto hex_to_int(const std::string &hex) -> int;

auto byte_array_to_hex(const unsigned char *data, unsigned length)
    -> std::string;

/**
 * @brief Helper-Method to convert an string into its hex-representation.
 *
 * @param data The string to be converted.
 *
 * @return Hex-encoded string
 */
auto string_to_hex(const std::string &in) -> std::string;

/**
 * @brief The method converts the string to hex (Hexadecimal) string
 *
 * @param[in] in the string that should be converted to hex (Hexadecimal) string
 * @param[in] length the length of string that should be converted to hex
 * (Hexadecimal) string
 * @return the converted hex (Hexadecimal) string
 */
auto charArrayToHex(const unsigned char *data, unsigned length) -> std::string;

/**
 * @brief The method converts hex (Hexadecimal) string to string
 *
 * @param[in] in the hex (Hexadecimal) string that should be converted to string
 * @param[in] data the converted hex to string
 * @return the converted string
 */
auto hexToCharArray(const std::string &in, unsigned char *data) -> int;

void hex_to_byte_array(const std::string &in, unsigned char *data);

/**
 * @brief Helper-Method to convert an hex-coded string into readable string.
 *
 * @param str The hex-encoded string to be converted.
 *
 * @return Readable string
 */
auto hex_to_string(const std::string &str) -> std::string;

#endif  // ENCODING_HELPERS_H
