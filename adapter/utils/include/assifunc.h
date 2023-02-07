#ifndef ASSIFUNC_H
#define ASSIFUNC_H

#include <curl/curl.h>

#include <boost/log/trivial.hpp>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <vector>

// size for buffer to get return after executing node command
#define BUFFER_SIZE_EXEC 128

#define ENCODED_BYTE_SIZE 16

// keys and values of smart contrat are 32 byte and represented as hex
// strings with 64 chars. Each byte as 2 chars
#define VALUE_SIZE 64

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

/**
 * @brief Helper-Method to convert an string into its hex-representation.
 *
 * @param data The string to be converted.
 *
 * @return Hex-encoded string
 */
auto string_to_hex(std::string &in) -> std::string;

/**
 * @brief Helper-Method to convert an hex-coded string into readable string.
 *
 * @param str The hex-encoded string to be converted.
 *
 * @return Readable string
 */
auto hex_to_string(const std::string &str) -> std::string;

#endif  // ASSIFUNC_H