#include "adapter_utils/encoding_helpers.h"

auto int_to_hex(size_t num, int size) -> std::string {
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(size) << std::hex << num;
  return stream.str();
}

auto hex_to_int(const std::string &hex) -> int {
  std::stringstream stream;
  stream << std::hex << hex;
  int i;
  stream >> i;
  return i;
}

auto byte_array_to_hex(const unsigned char *data, unsigned length)
    -> std::string {
  std::stringstream ss;
  ss << std::hex;
  unsigned i = 0;
  for (; i < length; i++) {
    ss << std::setw(2) << std::setfill('0') << (int)(data[i]);
  }

  return ss.str();
}

auto string_to_hex(const std::string &in) -> std::string {
  unsigned length = in.length();
  const unsigned char *buffer = (const unsigned char *)in.c_str();
  // buffer = &in[0];

  return byte_array_to_hex(buffer, length);
}

auto charArrayToHex(const unsigned char *data, unsigned length) -> std::string {
  return byte_array_to_hex(data, length);
}

auto hexToCharArray(const std::string &in, unsigned char *data) -> int {
  if ((in.length() % 2) != 0) {
    std::cout << "String is not valid length ...";
    return 1;
  }

  size_t cnt = in.length() / 2;

  for (size_t i = 0; cnt > i; ++i) {
    int s = 0;
    std::stringstream ss;
    ss << std::hex << in.substr(i * 2, 2);
    ss >> s;

    data[i] = (unsigned char)(s);
  }
  return 0;
}

void hex_to_byte_array(const std::string &in, unsigned char *data) {
  int error = hexToCharArray(in, data);
  if (error == 1) {
    throw std::runtime_error("String is not valid length ...");
  }
}

auto hex_to_string(const std::string &str) -> std::string {
  std::string output;

  for (unsigned int i = 0; i < str.length(); i += 2) {
    std::string byte_string = str.substr(i, 2);
    char c = (char)strtol(byte_string.c_str(), nullptr, ENCODED_BYTE_SIZE);
    if (c != '\0') {
      output += c;
    }
  }
  return output;
}
