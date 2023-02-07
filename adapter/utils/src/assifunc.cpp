#include "assifunc.h"

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

auto string_to_hex(const std::string &in) -> std::string {
  std::stringstream ss;

  ss << std::hex << std::setfill('0');
  for (char i : in) {
    ss << std::setw(2)
       << static_cast<unsigned int>(static_cast<unsigned char>(i));
  }

  return ss.str();
}
/*
auto string_to_hex(std::string &in) -> std::string {
  std::stringstream ss;
  ss << std::hex;
  unsigned long i = 0;
  for (; i < in.length(); i++) {
    ss << std::setw(2) << std::setfill('0') << (int)(in[i]);
  }

  for (; i < VALUE_SIZE / 2; i++) {
    ss << std::setw(2) << std::setfill('0')
       << 0;  // Fill string with zeros if input length < VALUE_SIZE
  }
  return ss.str();
}
*/

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

/*
auto hex_to_string(const std::string &in) -> std::string {
  std::string output;

  if ((in.length() % 2) != 0) {
    throw std::runtime_error("String is not valid length ...");
  }

  size_t cnt = in.length() / 2;

  for (size_t i = 0; cnt > i; ++i) {
    uint32_t s = 0;
    std::stringstream ss;
    ss << std::hex << in.substr(i * 2, 2);
    ss >> s;

    output.push_back(static_cast<char>(s));
  }

  return output;
}
*/