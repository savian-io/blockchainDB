#ifndef ADAPTER_INTERFACE_TEST_H
#define ADAPTER_INTERFACE_TEST_H

#include <gtest/gtest.h>
#include <sys/stat.h>

#include <array>
#include <filesystem>

#include "adapter_interface/adapter_interface.h"

// #include "adapter_factory/adapter_factory.h"

// Test-Fixture for adapter base tests. Test are implemented in
// bc_adapter_test.cpp

/**
 * @brief Test fixture for generic adapter tests. All adapter tests should
 * implement it.
 *
 * This [text
 * fixture](https://github.com/google/googletest/blob/master/docs/primer.md#test-fixtures-using-the-same-data-configuration-for-multiple-tests-same-data-multiple-tests)
 * provides generic setup and teardown logic that all adapter tests can use.
 * It implements generic tests that all new adapters should pass in the
 * corresponding .cpp file
 *
 */
class AdapterInterfaceTest
    : public testing::TestWithParam<
          std::tuple<std::shared_ptr<BcAdapter>, std::string, std::string>> {
 protected:
  /**
   * @brief Initialize any required variables for the test
   *
   */
  void SetUp() override {
    // Called before each test
    adapter_ = std::get<0>(GetParam());
    config_path_ = std::get<1>(GetParam());
    network_config_ = std::get<2>(GetParam());

    // Create folder "test_data" manually
    if (!std::filesystem::exists(test_folder_)) {
      std::filesystem::create_directories(test_folder_);
    }

    adapter_->init(config_path_, network_config_);
    adapter_->create_table(tablename_, tableAddress_);

    adapter_->put(key_val_map_);
  }

  /**
   * @brief Clean up after each test
   *
   */
  void TearDown() override {
    bool drop_table = adapter_->drop_table() == 0;
    if (drop_table) {
      std::cout << "\nA table can only be deleted once, so delete multiple "
                   "times: \" DROP_TABLE, Failed to delete File \" expect!! "
                << std::endl;
    }
    adapter_->shutdown();
    adapter_.reset();

    // delete all the files and folder, which created from Test
    if (std::filesystem::exists(test_folder_)) {
      // remove all files and subdirectories
      for (const auto &entry :
           std::filesystem::directory_iterator(test_folder_)) {
        std::filesystem::remove_all(entry.path());
      }

      // delete test folder "./test_data"
      std::filesystem::remove(test_folder_);
    }
  }

  //! Adapter instance that is used for the tests
  std::shared_ptr<BcAdapter> adapter_;
  //! path to the configuration file that the adapater uses
  std::string config_path_;
  //! network_config_ that the adapater uses
  std::string network_config_;

  /**********************************************
   *  Dummy data for tests
   ***********************************************/
  //!  The name of the folder in which all test results are saved
  std::string test_folder_ = "./test_data/db";
  //! Address of the table that the adapter will connect to
  std::string tableAddress_;
  //! Name of table used for tests
  std::string tablename_ = "test-table";

  //! Dummy key-value pairs for tests.
  std::map<const BYTES, const BYTES> key_val_map_ = {
      {{BYTES("key1")}, {BYTES("value1")}},
      {{BYTES("key2")}, {BYTES("value2")}},
      {{BYTES("key3")}, {BYTES("value3")}}};
  //! Dummy keys for tests. 1:1 mapping to values!
  std::array<const BYTES, 4> keys_ = {
      {{BYTES("key1")}, {BYTES("key2")}, {BYTES("key3")}, {BYTES("key4")}}};
  //! Dummy values for tests. 1:1 mapping to keys!
  std::array<const BYTES, 4> values_ = {{{BYTES("value1")},
                                         {BYTES("value2")},
                                         {BYTES("value3")},
                                         {BYTES("value4")}}};
  //! Dummmy map for testing the batch insert
  std::map<const BYTES, const BYTES> batch_ = {
      {{BYTES("AAAA")}, {BYTES("1111")}},
      {{BYTES("BBBB")}, {BYTES("2222")}},
      {{BYTES("CCCC")}, {BYTES("3333")}},
      {{BYTES("DDDD")}, {BYTES("4444")}},
      {{BYTES("EEEE")}, {BYTES("5555")}}};
  //! Dummy keys for testing the batch insert. 1:1 mapping to values!
  // NOLINTNEXTLINE(readability-magic-numbers)
  std::array<const BYTES, 5> batch_keys_ = {{{BYTES("AAAA")},
                                             {BYTES("BBBB")},
                                             {BYTES("CCCC")},
                                             {BYTES("DDDD")},
                                             {BYTES("EEEE")}}};
  //! Dummy values for testing the batch insert. 1:1 mapping to keys!
  // NOLINTNEXTLINE(readability-magic-numbers)
  std::array<const BYTES, 5> batch_values_ = {{{BYTES("1111")},
                                               {BYTES("2222")},
                                               {BYTES("3333")},
                                               {BYTES("4444")},
                                               {BYTES("5555")}}};

  /**********************************************
   *  Read buffers to store results
   ***********************************************/
  //! To store string results
  BYTES result_;
  //! For storing multiple result tuples
  std::map<const BYTES, BYTES> result_map_;
};
#endif  // ADAPTER_INTERFACE_TEST_H
