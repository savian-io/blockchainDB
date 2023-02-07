#ifndef ADAPTER_FABRIC_H
#define ADAPTER_FABRIC_H

#include <boost/log/trivial.hpp>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "adapter_interface/adapter_interface.h"
#include "client_fabric.h"
#include "config_fabric.h"

const std::string kSeparatorToken = "##";

/**
 * @brief BC_Adapter implementation for Hyperledger Fabric.
 *
 */
class FabricAdapter : public BcAdapter {
 public:
  explicit FabricAdapter();
  ~FabricAdapter() override;

  /**********************************************
   *  BC_Adapter methods to be implemented
   ***********************************************/

  //! @copydoc BcAdapter::init(const std::string &config_path)
  auto init(const std::string &config_path) -> bool override;
  auto init(const std::string &config_path, const std::string &network_config)
      -> bool override;
  auto check_connection() -> bool override;
  auto shutdown() -> bool override;

  auto put(std::map<const BYTES, const BYTES> &batch) -> int override;
  auto get(const BYTES &key, BYTES &result) -> int override;
  // clang-format off
  //! @copydoc BcAdapter::get_all(std::map<const BYTES, BYTES> &results)
  // clang-format on
  auto get_all(std::map<const BYTES, BYTES> &results) -> int override;
  auto remove(const BYTES &key) -> int override;

  auto create_table(const std::string &name, std::string &tableAddress)
      -> int override;
  auto load_table(const std::string &name, const std::string &tableAddress)
      -> int override;
  auto drop_table() -> int override;

 private:
  std::string tableName_;
  FabricConfig config_;

  //! FabricClient object to communicate with a blockchain
  FabricClient client_;
};
#endif  // ADAPTER_FABRIC_H
