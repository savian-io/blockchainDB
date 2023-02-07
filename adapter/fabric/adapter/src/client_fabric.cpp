#include "adapter_fabric/client_fabric.h"

#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "../extern/go_client/libclient.hpp"

// Helper methods

/**
 * @brief Transform a map of key-value pairs into a GoString containing
 * a JSON representation of the map
 *
 * @param in a map of key-value pairs
 *
 * @return key-value pairs represented as a GoString in JSON format
 */
static auto map_to_json_go_string(std::map<const BYTES, const BYTES>& bytes_map)
    -> GoString {
  nlohmann::json json_object = nlohmann::json::object();
  for (const auto& iter : bytes_map) {
    std::string key =
        BcAdapter::byte_array_to_hex(iter.first.value, iter.first.size);
    std::string value =
        BcAdapter::byte_array_to_hex(iter.second.value, iter.second.size);
    json_object[key] = value;
  }
  std::string serialized_map = json_object.dump();
  char* serialized_map_bytes = new char[serialized_map.length()];
  memcpy(serialized_map_bytes, serialized_map.c_str(), serialized_map.length());
  GoString serialized_go_map = {serialized_map_bytes,
                                (long)serialized_map.length()};
  return serialized_go_map;
}

/**
 * @brief Transform a JSON representation of a map of key-value pairs
 * into a map
 *
 * @param in key-value pairs represented as string in JSON format
 *
 * @return a map of key-value pairs
 */
static auto json_string_to_map(char* serialized_map)
    -> std::map<const BYTES, BYTES> {
  std::map<const BYTES, BYTES> bytes_map = std::map<const BYTES, BYTES>();
  nlohmann::json json_object = nlohmann::json::parse(serialized_map);
  for (const auto& iter : json_object.items()) {
    // BYTES implementation leads to unecessary allocation here
    // should BYTES be changed or should we not use the constructor here?
    auto key_length = iter.key().length() / 2;
    unsigned char* key_bytes = new unsigned char[key_length];
    BcAdapter::hex_to_byte_array(iter.key(), key_bytes);
    BYTES key = BYTES(key_bytes, iter.key().length() / 2);
    delete[] key_bytes;

    auto value_length = iter.value().get<std::string>().length() / 2;
    unsigned char* value_bytes = new unsigned char[value_length];
    BcAdapter::hex_to_byte_array(iter.value().get<std::string>(), value_bytes);
    BYTES value = BYTES(value_bytes, value_length);
    delete[] value_bytes;
    bytes_map[key] = value;
  }
  return bytes_map;
}

/**
 * @brief Return a GoString containing a JSON representation of an empty
 * object
 *
 * @return an empty object represented as a GoString in JSON format
 */
static auto empty_object_json_go_string() -> GoString {
  std::string empty_json_object = "{}";
  char* serialized_map_bytes = new char[empty_json_object.length()];
  memcpy(serialized_map_bytes, empty_json_object.c_str(),
         empty_json_object.length());
  GoString go_empty_json_object = {serialized_map_bytes,
                                   (long)empty_json_object.length()};
  return go_empty_json_object;
}

/**
 * @brief Transform bytes into a GoString containing a hex encoded
 * representation of the byte array
 *
 * @param in an array of bytes in a BYTES object
 *
 * @return GoString containing the bytes encoded as hex
 */
static auto bytes_to_go_string(const BYTES& in) -> GoString {
  std::string hex_representation =
      BcAdapter::byte_array_to_hex(in.value, in.size);
  char* hex_representation_bytes = new char[hex_representation.length()];
  memcpy(hex_representation_bytes, hex_representation.c_str(),
         hex_representation.length());
  GoString go_hex_representation = {hex_representation_bytes,
                                    (long)hex_representation.length()};
  return go_hex_representation;
}

/**
 * @brief Transform a string into a GoString
 *
 * @param in a C++ string
 *
 * @return a GoString
 */
static auto string_to_go_string(const std::string& in) -> GoString {
  char* string_bytes = new char[in.length()];
  memcpy(string_bytes, in.c_str(), in.length());
  GoString go_string = {string_bytes, (long)in.length()};
  return go_string;
}

// Constructor
FabricClient::FabricClient() = default;

// Destructor
FabricClient::~FabricClient() = default;

// Mapped contract methods
void FabricClient::init(std::string channel_name, std::string contract_name,
                        std::string msp_id, std::string cert_path,
                        std::string key_path, std::string tls_cert_path,
                        std::string peer_endpoint, std::string gateway_peer) {
  if (isInitializied_) {
    return;
  }
  this->channel_name_ = std::move(channel_name);
  this->contract_name_ = std::move(contract_name);
  this->msp_id_ = std::move(msp_id);
  this->cert_path_ = std::move(cert_path);
  this->key_path_ = std::move(key_path);
  this->tls_cert_path_ = std::move(tls_cert_path);
  this->peer_endpoint_ = std::move(peer_endpoint);
  this->gateway_peer_ = std::move(gateway_peer);

  isInitializied_ = true;
}

auto FabricClient::put(std::map<const BYTES, const BYTES>& batch) -> int {
  GoString key_value_pairs = map_to_json_go_string(batch);

  GoString msp_id = {this->msp_id_.c_str(), (long)this->msp_id_.length()};
  GoString cert_path = {this->cert_path_.c_str(),
                        (long)this->cert_path_.length()};
  GoString key_path = {this->key_path_.c_str(), (long)this->key_path_.length()};
  GoString tls_cert_path = {this->tls_cert_path_.c_str(),
                            (long)this->tls_cert_path_.length()};
  GoString peer_endpoint = {this->peer_endpoint_.c_str(),
                            (long)this->peer_endpoint_.length()};
  GoString gateway_peer = {this->gateway_peer_.c_str(),
                           (long)this->gateway_peer_.length()};
  GoString channel_name = {this->channel_name_.c_str(),
                           (long)this->channel_name_.length()};
  GoString contract_name = {this->contract_name_.c_str(),
                            (long)this->contract_name_.length()};

  GoString function = string_to_go_string("put");
  int status_code =
      Write(key_value_pairs, function, channel_name, contract_name, msp_id,
            cert_path, key_path, tls_cert_path, peer_endpoint, gateway_peer);
  delete[] key_value_pairs.p;
  delete[] function.p;
  return status_code;
}

auto FabricClient::get(const BYTES& key, BYTES& value) -> int {
  GoString go_key = bytes_to_go_string(key);

  GoString msp_id = {this->msp_id_.c_str(), (long)this->msp_id_.length()};
  GoString cert_path = {this->cert_path_.c_str(),
                        (long)this->cert_path_.length()};
  GoString key_path = {this->key_path_.c_str(), (long)this->key_path_.length()};
  GoString tls_cert_path = {this->tls_cert_path_.c_str(),
                            (long)this->tls_cert_path_.length()};
  GoString peer_endpoint = {this->peer_endpoint_.c_str(),
                            (long)this->peer_endpoint_.length()};
  GoString gateway_peer = {this->gateway_peer_.c_str(),
                           (long)this->gateway_peer_.length()};
  GoString channel_name = {this->channel_name_.c_str(),
                           (long)this->channel_name_.length()};
  GoString contract_name = {this->contract_name_.c_str(),
                            (long)this->contract_name_.length()};

  GoString function = string_to_go_string("get");
  Read_return go_result =
      Read(go_key, function, channel_name, contract_name, msp_id, cert_path,
           key_path, tls_cert_path, peer_endpoint, gateway_peer);
  delete[] go_key.p;
  delete[] function.p;
  if (go_result.r2 != 0) {
    return 1;
  }
  unsigned char* value_bytes = new unsigned char[go_result.r1 / 2];
  BcAdapter::hex_to_byte_array(go_result.r0, value_bytes);
  value = BYTES(value_bytes, go_result.r1 / 2);
  return 0;
}

auto FabricClient::getAll(std::map<const BYTES, BYTES>& values) -> int {
  GoString empty_json_object = empty_object_json_go_string();

  GoString msp_id = {this->msp_id_.c_str(), (long)this->msp_id_.length()};
  GoString cert_path = {this->cert_path_.c_str(),
                        (long)this->cert_path_.length()};
  GoString key_path = {this->key_path_.c_str(), (long)this->key_path_.length()};
  GoString tls_cert_path = {this->tls_cert_path_.c_str(),
                            (long)this->tls_cert_path_.length()};
  GoString peer_endpoint = {this->peer_endpoint_.c_str(),
                            (long)this->peer_endpoint_.length()};
  GoString gateway_peer = {this->gateway_peer_.c_str(),
                           (long)this->gateway_peer_.length()};
  GoString channel_name = {this->channel_name_.c_str(),
                           (long)this->channel_name_.length()};
  GoString contract_name = {this->contract_name_.c_str(),
                            (long)this->contract_name_.length()};

  GoString function = string_to_go_string("getAll");
  Read_return result =
      Read(empty_json_object, function, channel_name, contract_name, msp_id,
           cert_path, key_path, tls_cert_path, peer_endpoint, gateway_peer);
  delete[] empty_json_object.p;
  delete[] function.p;
  if (result.r2 != 0) {
    return 1;
  }
  values = json_string_to_map(result.r0);
  return 0;
}

auto FabricClient::remove(const BYTES& key) -> int {
  GoString go_key = bytes_to_go_string(key);

  GoString msp_id = {this->msp_id_.c_str(), (long)this->msp_id_.length()};
  GoString cert_path = {this->cert_path_.c_str(),
                        (long)this->cert_path_.length()};
  GoString key_path = {this->key_path_.c_str(), (long)this->key_path_.length()};
  GoString tls_cert_path = {this->tls_cert_path_.c_str(),
                            (long)this->tls_cert_path_.length()};
  GoString peer_endpoint = {this->peer_endpoint_.c_str(),
                            (long)this->peer_endpoint_.length()};
  GoString gateway_peer = {this->gateway_peer_.c_str(),
                           (long)this->gateway_peer_.length()};
  GoString channel_name = {this->channel_name_.c_str(),
                           (long)this->channel_name_.length()};
  GoString contract_name = {this->contract_name_.c_str(),
                            (long)this->contract_name_.length()};

  GoString function = string_to_go_string("delete");
  int status_code =
      Write(go_key, function, channel_name, contract_name, msp_id, cert_path,
            key_path, tls_cert_path, peer_endpoint, gateway_peer);
  delete[] go_key.p;
  delete[] function.p;
  return status_code;
}

void FabricClient::close() {
  isInitializied_ = false;
  this->channel_name_ = "";
  this->contract_name_ = "";
  this->msp_id_ = "";
  this->cert_path_ = "";
  this->key_path_ = "";
  this->tls_cert_path_ = "";
  this->peer_endpoint_ = "";
  this->gateway_peer_ = "";
}

// Public methods
auto FabricClient::isInit() const -> bool { return isInitializied_; }
