#ifndef CONFIG_ETHEREUM_H
#define CONFIG_ETHEREUM_H

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "adapter_interface/adapter_config.h"
//src/storage/blockchain/blockchain-adapter/interface/include/adapter_interface/adapter_interface.h
#include "storage/blockchainDB/adapter/utils/src/json.hpp"
#include "storage/blockchainDB/adapter/utils/include/general_helpers.h"

/**
 * @brief Define specific configuration values for the Ethereum adapter
 *
 */
class EthereumConfig : public AdapterConfig {
 public:
  /**
   * @brief Initialize the config bean by parsing the config file
   *
   * @param path Path to file to parse
   * @return true if parsing was successful otherwise false
   */
  auto init_path(const std::string& path) -> bool { return read(path); }

   /**
   * @brief Initialize adapter configuration
   *
   * @param mysql_data_dir Path to mysql data dir
   * @return True if initialization was successful otherwise false
   */
  auto set_adapter_config(const std::string& mysql_data_dir) -> bool override {
    // get max-waiting-time
    const int max_waiting_time = 300;
    BOOST_LOG_TRIVIAL(debug) << "set_adapter_config, max_waiting_time = "
                             << max_waiting_time;
    config_.put("Adapter-Ethereum.max_waiting_time", max_waiting_time);

    // get script-path
    std::string script_path = "";
    // add path to mysql data dir
    script_path.append(mysql_data_dir);
    // add relative path to script
    script_path.append("../../storage/blockchainDB/adapter/ethereum/scripts");
    BOOST_LOG_TRIVIAL(debug) << "set_adapter_config, script_path = "
                             << script_path;
    config_.put("Adapter-Ethereum.script_path", script_path);

    // get contract-path
    std::string contract_path = "";
    // add path to mysql data dir
    contract_path.append(mysql_data_dir);
    // add relative path to contract
    contract_path.append("../../storage/blockchainDB/adapter/ethereum/contract/truffle/build/contracts/SimpleStorage.json");
    BOOST_LOG_TRIVIAL(debug) << "set_adapter_config, contract_path = "
                             << contract_path;
    config_.put("Adapter-Ethereum.contract_path", contract_path);

    return true;
  }

  /**
   * @brief Initialize network configuration by parsing connection string
   *
   * @param config Connection string
   * @return True if parsing was successful otherwise false
   */
  auto set_network_config(const std::string& connection_string) -> bool override {
    // parse blockchain parameters from connection string
    auto connection_string_json = nlohmann::json::parse(connection_string);

    // get rpc-port
    const std::string rpc_port = connection_string_json["rpc-port"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, rpc-port = "
                             << rpc_port;
    config_.put("Adapter-Ethereum.rpc-port", rpc_port);

    // get join-ip
    const std::string join_ip = connection_string_json["join-ip"];
    BOOST_LOG_TRIVIAL(debug) << "set_network_config, join-ip = "
                             << join_ip;
    config_.put("Adapter-Ethereum.join-ip", join_ip);

    // create connection_url = "http://" + join_ip + ":" + rpc_port
    std::string connection_url = "http://" + join_ip + ":" + rpc_port;
    // set connection_url in adapter config
    config_.put("Adapter-Ethereum.connection-url", connection_url);
    return true;
  }

  /**
   * @brief rpc-port
   *
   * @return std::string
   */
  auto rpc_port() -> std::string {
    return config_.get<std::string>("Adapter-Ethereum.rpc-port");
  }

  /**
   * @brief The IP-Address of the Ethereum (geth) node to connect to
   *
   * @return std::string
   */
  auto connection_url() -> std::string {
    return config_.get<std::string>("Adapter-Ethereum.connection-url");
  }

  /**
   * @brief The address of (table) contract that the adapter will use for
   * reads/writes
   *
   * @return std::string
   */
  auto contract_address() -> std::string {
    return config_.get<std::string>("Adapter-Ethereum.contract-address");
  }

  /**
   * @brief The path to the compiled contract file that the adapter will use for
   * deploying the contract to the blockchain
   *
   * @return std::string
   */
  auto contract_path() -> std::string {
    return config_.get<std::string>("Adapter-Ethereum.contract_path");
  }

  /**
   * @brief The maximum time the adapter will wait for a blockchain TX to be
   * mined
   *
   * @return int
   */
  auto max_waiting_time() -> int {
    return config_.get<int>("Adapter-Ethereum.max_waiting_time");
  }

  /**
   * @brief Path to the folder containing the scripts to deploy contracts etc.
   *
   * @return std::string
   */
  auto script_path() -> std::string {
    return config_.get<std::string>("Adapter-Ethereum.script_path");
  }
};
#endif  // CONFIG_ETHEREUM_H
