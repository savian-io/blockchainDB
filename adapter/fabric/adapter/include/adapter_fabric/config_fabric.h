#ifndef CONFIG_FABRIC_H
#define CONFIG_FABRIC_H

#include "adapter_interface/adapter_config.h"
#include "storage/blockchainDB/adapter/utils/src/json.hpp"
#include "storage/blockchainDB/adapter/utils/include/general_helpers.h"

/**
 * @brief Define specific configuration values for the Fabric adapter
 *
 */
class FabricConfig : public AdapterConfig {
 public:
  /**
   * @brief Initialize the config bean by parsing the config file
   *
   * @param path Path to file to parse
   * @return true True if parsing was successful
   * @return false False if parsing error occurred, e.g., file not found
   */
  auto init(const std::string& path) -> bool { return read(path); }

  /**
   * @brief Initialize network configuration by parsing connection string
   *
   * @param config Connection string
   * @return True if parsing was successful otherwise false
   */
  auto set_network_config(const std::string& connection_string) 
      -> bool override {
    // parse blockchain parameters from connection string
    auto json = nlohmann::json::parse(connection_string);

    // get channel-name
    const std::string channel_name = json["channel_name"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, channel_name = "
                             << channel_name;
    config_.put("Adapter-Fabric.channel_name", channel_name);

    // get peer-port
    const std::string peer_port = json["peer_port"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, peer_port = "
                             << peer_port;
    config_.put("Adapter-Fabric.peer_port", peer_port);

    // get msp-id
    const std::string msp_id = json["msp_id"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, msp_id = "
                             << msp_id;
    config_.put("Adapter-Fabric.msp_id", msp_id);

    // get cert-path
    const std::string cert_path = json["cert_path"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, cert_path = "
                             << cert_path;
    config_.put("Adapter-Fabric.cert_path", cert_path);

    // get key-path
    const std::string key_path = json["key_path"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, key_path = "
                             << key_path;
    config_.put("Adapter-Fabric.key_path", key_path);

    // get tls-cert-path
    const std::string tls_cert_path = json["tls_cert_path"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, tls_cert_path = "
                             << tls_cert_path;
    config_.put("Adapter-Fabric.tls_cert_path", tls_cert_path);

    // get gateway-peer
    const std::string gateway_peer = json["gateway_peer"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, gateway_peer = "
                             << gateway_peer;
    config_.put("Adapter-Fabric.gateway_peer", gateway_peer);

    // get peer-endpoint
    const std::string peer_endpoint = json["peer_endpoint"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, peer_endpoint = "
                             << peer_endpoint;
    config_.put("Adapter-Fabric.peer_endpoint", peer_endpoint);

    // get test-network-path
    const std::string test_network_path = json["test_network_path"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, test_network_path = "
                             << test_network_path;
    config_.put("Adapter-Fabric.test_network_path", test_network_path);

    return true;
  }

  /**
   * @brief Path to the test network assets
   *
   * @return std::string
   */
  auto test_network_path() -> std::string {
    return config_.get<std::string>("Adapter-Fabric.test_network_path");
  }

  /**
   * @brief Path to blockchain-adapter folder
   *
   * @return std::string
   */
  auto adapters_path() -> std::string {
    return config_.get<std::string>("Adapter-Fabric.adapters-path");
  }

  /**
   * @brief Name of fabric network's channel
   *
   * @return std::string
   */
  auto channel_name() -> std::string {
    return config_.get<std::string>("Adapter-Fabric.channel_name");
  }

  /**
   * @brief Port of the peer that was added to the fabric network for
   * this instance
   *
   * @return std::string
   */
  auto peer_port() -> std::string {
    return config_.get<std::string>("Adapter-Fabric.peer_port");
  }

  /**
   * @brief The id of the membership service provider
   *
   * @return The msp id as a string
   */
  auto msp_id() -> std::string {
    return config_.get<std::string>("Adapter-Fabric.msp_id");
  }

  /**
   * @brief The path to the certificate of the client identity
   * used to transact with the network
   *
   * @return The path as a string
   */
  auto cert_path() -> std::string {
    return config_.get<std::string>("Adapter-Fabric.cert_path");
  }

  /**
   * @brief The path to a directoy with private keys of the client
   * used to transact with the network
   *
   * @return The path as a string
   */
  auto key_path() -> std::string {
    return config_.get<std::string>("Adapter-Fabric.key_path");
  }

  /**
   * @brief The path to the gateway peers tls certificate
   *
   * @return The path as a string
   */
  auto tls_cert_path() -> std::string {
    return config_.get<std::string>("Adapter-Fabric.tls_cert_path");
  }

  /**
   * @brief The gateway peer's name
   *
   * @return The name as a string
   */
  auto gateway_peer() -> std::string {
    return config_.get<std::string>("Adapter-Fabric.gateway_peer");
  }

  /**
   * @brief The address where the gateway peer can be reached
   *
   * @return The address as a string
   */
  auto peer_endpoint() -> std::string {
    return config_.get<std::string>("Adapter-Fabric.peer_endpoint");
  }
};
#endif  // CONFIG_FABRIC_H
