/* Copyright (c) 2004, 2020, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2.0,
  as published by the Free Software Foundation.

  This program is also distributed with certain software (including
  but not limited to OpenSSL) that is licensed under separate terms,
  as designated in a particular file or component or in included license
  documentation.  The authors of MySQL hereby grant you an additional
  permission to link the program and your derivative works with the
  separately licensed software that they have included with MySQL.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License, version 2.0, for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/**
  @file ha_blockchain.cc

  @brief
  The ha_blockchain engine is a stubbed storage engine for blockchain purposes
  only; it does nothing at this point. Its purpose is to provide a source code
  illustration of how to begin writing new storage engines; see also
  /storage/blockchain/ha_blockchain.h.

  @details
  ha_blockchain will let you create/open/delete tables, but
  nothing further (for blockchain, indexes are not supported nor can data
  be stored in the table). Use this blockchain as a template for
  implementing the same functionality in your own storage engine. You
  can enable the blockchain storage engine in your build by doing the
  following during your build process:<br> ./configure
  --with-blockchain-storage-engine

  Once this is done, MySQL will let you create tables with:<br>
  CREATE TABLE \<table name\> (...) ENGINE=BLOCKCHAIN;

  The blockchain storage engine is set up to use table locks. It
  implements an example "SHARE" that is inserted into a hash by table
  name. You can use this to store information of state that any
  blockchain handler object will be able to see when it is using that
  table.

  Please read the object definition in ha_blockchain.h before reading the rest
  of this file.

  @note
  When you create an BLOCKCHAIN table, the MySQL Server creates a table .frm
  (format) file in the database directory, using the table name as the file
  name as is customary with MySQL. No other files are created. To get an idea
  of what occurs, here is an example select that would do a scan of an entire
  table:

  @code
  ha_blockchain::store_lock
  ha_blockchain::external_lock
  ha_blockchain::info
  ha_blockchain::rnd_init
  ha_blockchain::extra
  ha_blockchain::rnd_next
  ha_blockchain::rnd_next
  ha_blockchain::rnd_next
  ha_blockchain::rnd_next
  ha_blockchain::rnd_next
  ha_blockchain::rnd_next
  ha_blockchain::rnd_next
  ha_blockchain::rnd_next
  ha_blockchain::rnd_next
  ha_blockchain::extra
  ha_blockchain::external_lock
  ha_blockchain::extra
  ENUM HA_EXTRA_RESET        Reset database to after open
  @endcode

  Here you see that the blockchain storage engine has 9 rows called before
  rnd_next signals that it has reached the end of its data. Also note that
  the table in question was already opened; had it not been open, a call to
  ha_blockchain::open() would also have been necessary. Calls to
  ha_blockchain::extra() are hints as to what will be occurring to the request.

  A Longer Example can be found called the "Skeleton Engine" which can be
  found on TangentOrg. It has both an engine and a full build environment
  for building a pluggable storage engine.

  Happy coding!<br>
    -Brian
*/

#include "blockchain/ha_blockchain.h"
#include "trustdble_utils/encoding_helpers.h"
#include "trustdble_utils/general_helpers.h"

#include <sql/sql_thd_internal_api.h>
#include <sql/table.h>
#include <iostream>
#include <vector>

//#include "my_dbug.h"
#include <fstream>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <filesystem>
#include <regex>
#include <set>
#include <unordered_map>
#include "blockchain/crypt_service.h"
#include "my_sys.h"
#include "mysql/components/services/log_builtins.h"
#include "mysql/plugin.h"
#include "sql/field.h"
#include "sql/sql_base.h"
#include "sql/sql_class.h"
#include "sql/sql_plugin.h"
#include "sql/transaction.h"
#include "typelib.h"

#ifdef RAPIDJSON_NO_SIZETYPEDEFINE
// if we build within the server, it will set RAPIDJSON_NO_SIZETYPEDEFINE
// globally and require to include my_rapidjson_size_t.h
#include "my_rapidjson_size_t.h"
#endif

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"

using namespace rapidjson;
handlerton *blockchain_hton;
// Map of bc_adapters for open tables, <full_table_name, bc_adapter>
static std::unordered_map<std::string, std::unique_ptr<BcAdapter>>
    bc_adapter_map;

// LOG-Tag for this class
#define LOG_TAG "blockchain"

// System variables for configuration
static char *config_configuration_path;

/* Interface to mysqld, to check system tables supported by SE */
static bool blockchain_is_supported_system_table(
    const char *db, const char *table_name, bool is_sql_layer_system_table);

// Init function for plugin
static int blockchain_init_func(void *p) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: blockchain_init_func"));
  blockchain_hton = (handlerton *)p;
  blockchain_hton->state = SHOW_OPTION_YES;
  blockchain_hton->create = ha_blockchain::bc_create_handler;
  blockchain_hton->flags = HTON_CAN_RECREATE | HTON_ALTER_NOT_SUPPORTED;
  blockchain_hton->is_supported_system_table =
      blockchain_is_supported_system_table;
  blockchain_hton->commit = ha_blockchain::bc_commit;
  blockchain_hton->rollback = ha_blockchain::bc_rollback;
  blockchain_hton->close_connection = ha_blockchain::bc_close_connection;

  return 0;
}

/*
  List of all system tables specific to the SE.
  Array element would look like below,
     { "<database_name>", "<system table name>" },
  The last element MUST be,
     { (const char*)NULL, (const char*)NULL }

  This array is optional, so every SE need not implement it.
*/
static st_handler_tablename ha_blockchain_system_tables[] = {
    {(const char *)nullptr, (const char *)nullptr}};

/**
  @brief Check if the given db.tablename is a system table for this SE.

  @param db                         Database name to check.
  @param table_name                 table name to check.
  @param is_sql_layer_system_table  if the supplied db.table_name is a SQL
                                    layer system table.

  @retval true   Given db.table_name is supported system table.
  @retval false  Given db.table_name is not a supported system table.
*/
static bool blockchain_is_supported_system_table(
    const char *db, const char *table_name, bool is_sql_layer_system_table) {
  DBUG_PRINT(
      LOG_TAG,
      ("ha_blockchain_method_call: blockchain_is_supported_system_table"));
  st_handler_tablename *systab;

  // Does this SE support "ALL" SQL layer system tables ?
  if (is_sql_layer_system_table) return false;

  // Check if this is SE layer system tables
  systab = ha_blockchain_system_tables;
  while (systab && systab->db) {
    if (systab->db == db && strcmp(systab->tablename, table_name) == 0)
      return true;
    systab++;
  }

  return false;
}

/*
  Parse blockchain connection info from table->s->connect_string
*/
static int parse_connection_str(MEM_ROOT *mem_root, BC_TABLE *share,
                                TABLE *table) {
  DBUG_PRINT(LOG_TAG, ("parse_connection_str") );
  DBUG_PRINT(LOG_TAG, ("share at %p", share) );
  // lenght of connection string
  DBUG_PRINT(LOG_TAG, ("length = %u", (uint)table->s->connect_string.length) );
  // connection string
  DBUG_PRINT(LOG_TAG, ("connection string = '%.*s'",
      (int)table->s->connect_string.length, table->s->connect_string.str) );

  share->connection_string = strmake_root(
      mem_root, table->s->connect_string.str, table->s->connect_string.length);

  DBUG_PRINT(LOG_TAG, (
      "parse_connection_str alloced share->connection_string %p",
      share->connection_string) );
  // get connection string
  DBUG_PRINT(LOG_TAG, ("share->connection_string %s",
      share->connection_string) );

  return 0;
}

/**
 * @brief Get path to file with bc-table metadata
 *
 * @param[in] db_name Database name
 * @param[in] table_name Table name
 *
 * @return Path path to file with bc-table metadata
 */
std::string get_path_to_file_with_table_metadata(
    const std::string &db_name,
    const std::string &table_name) {
  
  // get path to dir with metadata
  std::string path_to_dir = "";
  // similar to configuration path
  path_to_dir.append(config_configuration_path);
  // cut tail
  const std::string tail_to_cut = "/configs/configuration.ini";
  boost::replace_all(path_to_dir, tail_to_cut, "");
  // add database name
  path_to_dir.append("data/");
  path_to_dir.append(db_name);
  path_to_dir.append("/");

  // add table name
  /*In fact the SDI file is a compact JSON file.
    It’s name is formed by the table's name and the table’s id.*/
  // template to compare file names
  std::string reg_template = "";
  reg_template.append(table_name);
  reg_template.append("_");
  reg_template.append("[0-9]+\\.sdi");
  std::regex file_regex(reg_template);
  std::string path_to_file = "";

  // get each file name in directory
  for (const auto & entry : std::filesystem::directory_iterator(path_to_dir)) {
    path_to_file = entry.path().string();
    std::string file_name = path_to_file;
    boost::replace_all(file_name, path_to_dir, "");
    // stop if file name matches table name
    if ( std::regex_match(file_name, file_regex) ) {
      DBUG_PRINT(LOG_TAG,(
          "get_path_to_file_with_table_metadata: file_name = %s",
          file_name.c_str() ));
      break;
    }
  }
  DBUG_PRINT(LOG_TAG,(
      "get_path_to_file_with_table_metadata: path_to_file = %s",
      path_to_file.c_str() ));

  return path_to_file;
}

/**************************
 * Storage engine methods *
 **************************/

// Create blockchain handler
handler *ha_blockchain::bc_create_handler(handlerton *hton, TABLE_SHARE *table,
                                          bool, MEM_ROOT *mem_root) {
  return new (mem_root) ha_blockchain(hton, table);
}

// Commit transaction
int ha_blockchain::bc_commit(handlerton *, THD *thd, bool commit_trx) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: bc_commit"));

  // test_print
  for (auto it = bc_adapter_map.cbegin(); it != bc_adapter_map.cend(); ++it) {
    DBUG_PRINT(LOG_TAG,
               ("bc_commit: bc_adapter_map_key = %s", (*it).first.c_str() ));
  }

  if (!commit_trx &&
      thd_test_options(thd, (OPTION_NOT_AUTOCOMMIT | OPTION_BEGIN))) {
    // Statement commit but in a transaction so nothing to do
    DBUG_PRINT(LOG_TAG, ("COMMIT: Nothing to commit"));
    return 0;
  }

  // Get transaction from memory of the handler
  Transaction *txn = static_cast<Transaction *>(
      thd->get_ha_data(blockchain_hton->slot)->ha_ptr);

  if (txn == nullptr) return 0;

  // intermediate data structure for batching write transactions.for every table
  // we store all the write statements that operate on that table.
  std::map<std::string, std::map<const BYTES, const BYTES>> write_batch_map;
  // Loop over all statements and send them to the blockchain
  for (unsigned int i = 0; i < txn->statements.size(); i++) {
    std::string full_table_name = std::string(txn->statements[i].tablename);
    std::string database_name =
        full_table_name.substr(2, full_table_name.find_last_of('/') - 2);
    std::string table_name = full_table_name.substr(
        full_table_name.find_last_of('/') + 1, full_table_name.length());

    // for tables on data_chain
    std::string key_hex = byte_array_to_hex(txn->statements[i].key.value,
                                              txn->statements[i].key.size);
    DBUG_PRINT(LOG_TAG, ("bc_commit: i = %d", i));
    DBUG_PRINT(LOG_TAG, ("bc_commit: txn->statements[i].key.c_str() = %s",
                          key_hex.c_str()));

    std::string key_str = key_hex.c_str();
    // define bc_adapter_map_key for table name
    std::string bc_adapter_map_key = full_table_name;
    DBUG_PRINT(LOG_TAG, ("bc_commit: bc_adapter_map_key = %s",
                          bc_adapter_map_key.c_str()));

    auto it = bc_adapter_map.find(bc_adapter_map_key);

    if (it == bc_adapter_map.end()) {
      std::string tablename = std::string(txn->statements[i].tablename);
      DBUG_PRINT(LOG_TAG,
                 ("BC_COMMIT: can't find bc_adapter for table_name = %s",
                  tablename.c_str()));
      return 1;
    }
    std::map<std::string, std::map<const BYTES, const BYTES>>::iterator
        table_it = write_batch_map.find(bc_adapter_map_key);
    if (table_it == write_batch_map.end()) {
      std::map<const BYTES, const BYTES> write_batch;
      write_batch_map.emplace(bc_adapter_map_key, write_batch);
      table_it = write_batch_map.find(bc_adapter_map_key);
    }
    // High level logic of the following lines:
    // Assume the following transaction order (W=Write, R=Remove): W1, W2, R1,
    // W3, W4, W5, R2 In this case, we batch W1 and W2 and send them together to
    // the blockchain; then R1 is executed Afterwards, W3-W5 are batched and
    // sent to the blockchain; lastly, R2 is executed
    if (txn->statements[i].type == STATEMENT_TYPE::WRITE) {
      size_t value_size = txn->statements[i].value.size;
      // create BYTES struct from value
      BYTES value_bytes = BYTES(txn->statements[i].value.value, value_size);
      table_it->second.emplace(txn->statements[i].key, value_bytes);
    } else if (txn->statements[i].type == STATEMENT_TYPE::REMOVE) {
      // If a remove statement occurs, then first check if we have elements in
      // the batch Add the elements in the batch to the blockchain and then
      // clear the batch
      if (table_it->second.size() != 0) {
        it->second->put(table_it->second);
        //if (!(it->second->put(table_it->second))) {
          // blockchain network is NOT available
        //  DBUG_PRINT(LOG_TAG,("bc_commit: blockchain network is NOT available"));
        //  return 1;
        //}
        table_it->second.clear();
      }
      // Then the remove statement is executed afterwards
      it->second->remove(txn->statements[i].key);
    }
  }
  // send all of the elements of write_batch to the blockchain
  for (auto table_it = write_batch_map.begin();
       table_it != write_batch_map.end(); table_it++) {
    if (table_it->second.size() != 0) {
      auto it = bc_adapter_map.find(table_it->first);
      it->second->put(table_it->second);
      //if (!(it->second->put(table_it->second))) {
        // blockchain network is NOT available
      //  DBUG_PRINT(LOG_TAG,("bc_commit: blockchain network is NOT available"));
      //  return 1;
      //}
    }
  }
  // Remove transaction
  delete txn;
  thd->get_ha_data(blockchain_hton->slot)->ha_ptr = nullptr;
  return 0;
}

// Rollback transaction
int ha_blockchain::bc_rollback(handlerton *, THD *thd, bool all) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: bc_rollback"));
  if (!all && thd_test_options(thd, (OPTION_NOT_AUTOCOMMIT | OPTION_BEGIN))) {
    DBUG_PRINT(LOG_TAG, ("ROLLBACK: nothing to rollback"));
    return 0;
  }

  // Get transaction from memory of the handler
  Transaction *txn = static_cast<Transaction *>(
      thd->get_ha_data(blockchain_hton->slot)->ha_ptr);

  if (txn == nullptr) return 0;

  DBUG_PRINT(LOG_TAG, ("ROLLBACK Transaction"));

  // Remove transaction
  delete txn;
  thd->get_ha_data(blockchain_hton->slot)->ha_ptr = nullptr;
  return 0;
}

// Close connection
int ha_blockchain::bc_close_connection(handlerton *hton, THD *thd) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: bc_close_connection"));
  std::cout << "[BLOCKCHAIN] Closing connection with THD ID "
            << thd->thread_id() << std::endl;

  (void)hton;
  // Free HA Data Map
  // auto ha_data = thd->get_ha_data(hton->slot);
  // delete static_cast<ha_data_map *>(ha_data->ha_ptr);
  return 0;
}

/********************************************
 * Blockchain handler implementation *
 ********************************************/

ha_blockchain::ha_blockchain(handlerton *hton, TABLE_SHARE *table_arg)
    : handler(hton, table_arg) {
  // DBUG_TRACE;
  // DBUG_PRINT(LOG_TAG, ("Constructor:"));
}

ha_blockchain::~ha_blockchain() {  // DBUG_PRINT(LOG_TAG, ("Destructor:"));
}

/**
  @brief
  create() is called to create a table. The variable name will have the name
  of the table.

  @details
  When create() is called you do not need to worry about
  opening the table. Also, the .frm file will have already been
  created so adjusting create_info is not necessary. You can overwrite
  the .frm file at this point if you wish to change the table
  definition, but there are no methods currently provided for doing
  so.

  Called from handle.cc by ha_create_table().

  @see
  ha_create_table() in handle.cc
*/
int ha_blockchain::create(const char *name, TABLE *table_arg, HA_CREATE_INFO *,
                          dd::Table *) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: create"));
  DBUG_PRINT(LOG_TAG, ("create: new shared table with name= %s", name));

  // get database name
  std::string db_name = std::string(name);
  db_name = db_name.substr(db_name.find_first_of('/') + 1,
      db_name.find_last_of('/') - (db_name.find_first_of('/') + 1));
  DBUG_PRINT(LOG_TAG, ("create: db_name = %s", db_name.c_str() ));

  // get table name
  std::string table_name = std::string(name);
  table_name = table_name.substr(table_name.find_last_of('/') + 1,
      table_name.length());
  DBUG_PRINT(LOG_TAG, ("create: table_name = %s", table_name.c_str() ));

  // parse connection string from create statement options
  THD *thd = current_thd;
  // bc_table stores metadata of bc table
  BC_TABLE bc_table;
  parse_connection_str(thd->mem_root, &bc_table, table_arg);
  DBUG_PRINT(LOG_TAG, ("create: bc_table.connection_string = %s",
                       bc_table.connection_string));

  std::string connection_string(bc_table.connection_string);
  DBUG_PRINT(LOG_TAG, ("create: connection_string = %s",
                       connection_string.c_str() ));

  // check if connection string is valid JSON
  if ( !nlohmann::json::accept(connection_string) ) {
    // create adapter failed
    DBUG_PRINT(LOG_TAG,("create: FAILED, connection string %s is NOT valid JSON",
                        connection_string.c_str() ));
    return 1;
  }

  // parse blockchain type from connection string
  auto connection_str_as_json = nlohmann::json::parse(connection_string);
  const std::string bc_type = connection_str_as_json["bc_type"];
  DBUG_PRINT(LOG_TAG,( "create: bc_type = %s", bc_type.c_str() ));

  // table address
  std::string table_address = "";
 
  // check if table address is in connection string
  if ( boost::algorithm::contains(connection_string, "table_address") ) {
    // join bc-table
    DBUG_PRINT(LOG_TAG,( "create: JOIN bc-table %s", table_name.c_str() ));
    // parse table address from connection string
    std::string table_address = connection_str_as_json["table_address"];
    DBUG_PRINT(LOG_TAG,( "create: table_address = %s", table_address.c_str() ));
  } else {
    // create bc-table
    DBUG_PRINT(LOG_TAG,( "create: CREATE bc-table %s", table_name.c_str() ));

    // create new adapter of type bc_type
    std::unique_ptr<BcAdapter> bc_adapter;
    if ( (bc_adapter = AdapterFactory::create_adapter(
              AdapterFactory::getBC_TYPE(bc_type)) ) != nullptr ) {

      // config configuration path
      DBUG_PRINT(LOG_TAG, ("create: config_configuration_path = %s",
                           config_configuration_path));

      // initialize adapter
      DBUG_PRINT(LOG_TAG, ("create: initialize bc adapter"));
      if (!bc_adapter->init(config_configuration_path,
                            bc_table.connection_string)) {
        // initialize adapter failed
        DBUG_PRINT(LOG_TAG,("create: initialize bc adapter failed"));
        return 1;
      }

      // create bc-table and return table address
      bc_adapter->create_table(table_name, table_address);
      DBUG_PRINT(LOG_TAG, ("create: address for table %s is: %s",
                           table_name.c_str(), table_address.c_str() ));
    } else {
      // create adapter failed
      DBUG_PRINT(LOG_TAG,("create: failed, can not create adapter of type %s",
                          bc_type.c_str() ));
      return 1;
    }
  }

  // file with table metadata (connection string) is already created
  // update metadata by adding table address to connection string
  // and rewrite metadata file
  //std::string connection_string_data = table->s->connect_string.str;
  //DBUG_PRINT(LOG_TAG,("create: connection_str_as_json = %s",
  //                    connection_str_as_json.dump().c_str() ));
  // add table address to connection string
  connection_str_as_json["table_address"] = table_address;
  DBUG_PRINT(LOG_TAG,("create: connection string with table address = %s",
                      connection_str_as_json.dump().c_str() ));

  // file with table metadata
  std::fstream metadata_file;
  
  // get path to file with bc-table metadata
  std::string path_to_file = get_path_to_file_with_table_metadata(db_name,
                                                                  table_name);

  // open file to read
  metadata_file.open(path_to_file, ios::in);
  std::string table_metadata = "";
  if ( metadata_file.is_open() ) {
    std::getline(metadata_file, table_metadata);
    // update table_metadata by replacing {\} with {}
    boost::replace_all(table_metadata, "\\", "");
    //DBUG_PRINT(LOG_TAG,("create: metadata file = %s",
    //                    table_metadata.c_str() ));
  }
  else {
    DBUG_PRINT(LOG_TAG,("create: failed, can not open metadata file"));
  }
  // close
  metadata_file.close();

  // get substring connection string from table metadata
  size_t pos_start = table_metadata.find("connection_string=");
  std::string table_metadata_cut = table_metadata.substr(
      pos_start + "connection_string="s.size());
  size_t pos_end = table_metadata_cut.find(";");
  std::string connection_string_data = table_metadata_cut.substr(0, pos_end);
  DBUG_PRINT(LOG_TAG,("create: connection_string_data = %s",
                      connection_string_data.c_str() ));
  // update table metadata by adding table address to connection string
  boost::replace_all(table_metadata, connection_string_data,
                     connection_str_as_json.dump());
  DBUG_PRINT(LOG_TAG,("create: updated metadata file = %s",
                      table_metadata.c_str() ));

  // open file to write
  metadata_file.open(path_to_file, ios::out);
  metadata_file << table_metadata;
  // close
  metadata_file.close();

  return 0;
}

/**
  @brief
  Used for opening tables. The name will be the name of the file.

  @details
  A table is opened when it needs to be opened; e.g. when a request comes in
  for a SELECT on the table (tables are not open and closed for each request,
  they are cached).

  Called from handler.cc by handler::ha_open(). The server opens all tables by
  calling ha_open() which then calls the handler specific open().

  @see
  handler::ha_open() in handler.cc
*/
int ha_blockchain::open(const char *full_table_name, int, uint,
                        const dd::Table *) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: open"));

  // get database name
  std::string db_name = std::string(full_table_name);
  db_name = db_name.substr(db_name.find_first_of('/') + 1,
      db_name.find_last_of('/') - (db_name.find_first_of('/') + 1));
  DBUG_PRINT(LOG_TAG, ("open: db_name = %s", db_name.c_str() ));

  // get table name
  std::string table_name = std::string(full_table_name);
  table_name = table_name.substr(table_name.find_last_of('/') + 1,
      table_name.length());
  DBUG_PRINT(LOG_TAG, ("open: table_name = %s", table_name.c_str() ));

  // file with table metadata
  std::fstream metadata_file;

  // get path to file with bc-table metadata
  std::string path_to_file = get_path_to_file_with_table_metadata(db_name,
                                                                  table_name);

  // open file to read
  metadata_file.open(path_to_file, ios::in);
  std::string table_metadata = "";
  if ( metadata_file.is_open() ) {
    std::getline(metadata_file, table_metadata);
    //DBUG_PRINT(LOG_TAG,("open: table_metadata = %s",
    //                    table_metadata.c_str() ));
  }
  else {
    DBUG_PRINT(LOG_TAG,("open: failed, can not open metadata file"));
  }
  // close
  metadata_file.close();

  // get substring connection string from table metadata
  size_t pos_start = table_metadata.find("connection_string=");
  table_metadata = table_metadata.substr(pos_start + "connection_string="s.size());
  size_t pos_end = table_metadata.find(";");
  std::string connection_string = table_metadata.substr(0, pos_end);
  boost::replace_all(connection_string, "\\\\", "");
  boost::replace_all(connection_string, "\\", "");
  DBUG_PRINT(LOG_TAG,("open: connection_string = %s",
                      connection_string.c_str() ));

  // check if connection string is valid JSON
  if ( !nlohmann::json::accept(connection_string) ) {
    // create adapter failed
    DBUG_PRINT(LOG_TAG,("open: FAILED, connection string [%s] is NOT valid JSON",
                        connection_string.c_str() ));
    return 1;
  }
  // parse connectin string
  auto connection_str_as_json = nlohmann::json::parse(connection_string);
  // parse blockchain type from connection string
  const std::string bc_type = connection_str_as_json["bc_type"];
  DBUG_PRINT(LOG_TAG,( "open: bc_type = %s", bc_type.c_str() ));
  // parse table address from connection string
  const std::string table_address = connection_str_as_json["table_address"];
  DBUG_PRINT(LOG_TAG,( "open: table_address = %s", table_address.c_str() ));
                   
  // create new adapter of type bc_type
  std::unique_ptr<BcAdapter> bc_adapter;
  if ( (bc_adapter = AdapterFactory::create_adapter(
           AdapterFactory::getBC_TYPE(bc_type))) != nullptr ) {

    // initialize adapter
    DBUG_PRINT(LOG_TAG, ("open: initialize bc adapter"));
    if (!bc_adapter->init(config_configuration_path, connection_string)) { 
      // initialize adapter failed
      DBUG_PRINT(LOG_TAG,("open: initialize bc adapter failed"));
      return 1;
    }

    // load table
    bc_adapter->load_table(table_name, table_address);
    DBUG_PRINT(LOG_TAG, ("open: opening table %s with address: %s",
                         table_name.c_str(), table_address.c_str() ));

    // add adapter to adapter map
    std::string bc_adapter_map_key = full_table_name;
    bc_adapter_map.emplace(bc_adapter_map_key, std::move(bc_adapter));
    // test print bc_adapter_map
    for (auto it = bc_adapter_map.cbegin(); it != bc_adapter_map.cend(); ++it) {
      DBUG_PRINT(LOG_TAG,(
          "open: bc_adapter_map_key = %s", (*it).first.c_str() ));
    }
  } else {
    // create adapter failed
    DBUG_PRINT(LOG_TAG,("open: failed, can not create adapter of type %s",
                         bc_type.c_str() ));
    return 1;
  }
  return 0;
}

/**
  @brief
  Closes a table.

  @details
  Called from sql_base.cc, sql_select.cc, and table.cc. In sql_select.cc it is
  only used to close up temporary tables or during the process where a
  temporary table is converted over to being a myisam table.

  For sql_base.cc look at close_data_tables().

  @see
  sql_base.cc, sql_select.cc and table.cc
*/
int ha_blockchain::close() {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: close"));
  DBUG_PRINT(LOG_TAG, ("CLOSE: Table = %s", table->s->table_name.str));
  std::stringstream full_table_name;
  full_table_name << "./";
  full_table_name << table->s->db.str;
  full_table_name << "/";
  full_table_name << table->s->table_name.str;

  // Delete Adapter from map
  auto it = bc_adapter_map.find(full_table_name.str());
  if (it != bc_adapter_map.end()) {
    it->second->shutdown();
    bc_adapter_map.erase(it);
  }
  return 0;
}

auto ha_blockchain::get_primary_key(const uchar *buf) -> BYTES {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: get_primary_key"));
  Field *key_field;
  uint initial_null_bytes = table->s->null_bytes;
  unsigned char key_hash[HASH_SIZE];
  unsigned int hash_size;
  ulong key_size = 0;
  // uchar *temp_buf = const_cast<uchar *>(buf);
  if (table->key_info != nullptr) {
    // allocate memory for the new pointer => MySQL library for allocating
    // memory
    // used
    uchar *key_adj = (uchar *)my_malloc(
        0, (sizeof(buf) * table->key_info[table->s->primary_key].key_length),
        MYF(0));
    uint16 initial_pos = 0;
    for (uint i = 0;
         i < table->key_info[table->s->primary_key].user_defined_key_parts;
         i++) {
      key_field = table->key_info[table->s->primary_key].key_part[i].field;
      key_size = key_field->pack_length();
      uint16 key_pos = key_field->offset(const_cast<uchar *>(buf));
      memcpy(key_adj + initial_pos, buf + key_pos, key_size);
      initial_pos += key_size;
      // DBUG_PRINT(LOG_TAG, ("BCStorageEngine: KEY-LENGTH:%lu", key_size));
    }

    key_size = initial_pos;
    hash_sha256(key_adj, key_size, key_hash, &hash_size);
    my_free(key_adj);
  } else {
    // if our table has no default value we use the first column as the key
    key_field = *(table->field);
    key_size = key_field->pack_length();
    // DBUG_PRINT(LOG_TAG, ("BCStorageEngine: KEY-LENGTH:%lu", key_size));
    hash_sha256(buf + initial_null_bytes, key_size, key_hash, &hash_size);
  }
  return BYTES(key_hash, hash_size);
}

/**
  @brief
  write_row() inserts a row. No extra() hint is given currently if a bulk load
  is happening. buf() is a byte array of data. You can use the field
  information to extract the data from the native byte array type.

  @details
  Blockchain of this would be:
  @code
  for (Field **field=table->field ; *field ; field++)
  {
    ...
  }
  @endcode

  See ha_tina.cc for an example of extracting all of the data as strings.
  ha_berekly.cc has an example of how to store it intact by "packing" it
  for ha_berkeley's own native storage type.

  See the note for update_row() on auto_increments. This case also applies to
  write_row().

  Called from item_sum.cc, item_sum.cc, sql_acl.cc, sql_insert.cc,
  sql_insert.cc, sql_select.cc, sql_table.cc, sql_udf.cc, and sql_update.cc.

  @see
  item_sum.cc, item_sum.cc, sql_acl.cc, sql_insert.cc,
  sql_insert.cc, sql_select.cc, sql_table.cc, sql_udf.cc and sql_update.cc
*/
int ha_blockchain::write_row(uchar *buf) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: write_row"));
  // Key and value as strings in hex
  std::string key;

  // Nullbytes of a row
  uint initial_null_bytes = table->s->null_bytes;

  // Set key
  BYTES key_bytes = get_primary_key(buf);

  // Set value
  BYTES value_bytes = BYTES((buf + initial_null_bytes),
                            table->s->reclength - initial_null_bytes);
  // Add write statement to transaction
  Transaction *txn = static_cast<Transaction *>(
      ha_thd()->get_ha_data(blockchain_hton->slot)->ha_ptr);
  std::stringstream full_table_name;
  full_table_name << "./";
  full_table_name << table->s->db.str;
  full_table_name << "/";
  full_table_name << table->s->table_name.str;
  if (txn->table_cache.at(full_table_name.str()).find(key_bytes) !=
      txn->table_cache.at(full_table_name.str()).end()) {
    return HA_ERR_WRONG_COMMAND;
  }
  txn->addWrite(full_table_name.str(), key_bytes, value_bytes);
  // Execute write in table cache of transaction
  txn->table_cache.at(full_table_name.str())[key_bytes] = value_bytes;
  return 0;
}

/**
  @brief
  Yes, update_row() does what you expect, it updates a row. old_data will have
  the previous row record in it, while new_data will have the newest data in it.
  Keep in mind that the server can do updates based on ordering if an ORDER BY
  clause was used. Consecutive ordering is not guaranteed.

  @details
  Currently new_data will not have an updated auto_increment record. You can
  do this for example by doing:

  @code

  if (table->next_number_field && record == table->record[0])
    update_auto_increment();

  @endcode

  Called from sql_select.cc, sql_acl.cc, sql_update.cc, and sql_insert.cc.

  @see
  sql_select.cc, sql_acl.cc, sql_update.cc and sql_insert.cc
*/
int ha_blockchain::update_row(const uchar *old_data, uchar *new_data) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: update_row"));

  // Nullbytes of a row
  uint initial_null_bytes = table->s->null_bytes;
  BYTES key_bytes_new = get_primary_key(new_data);

  // Set value
  BYTES new_value_bytes = BYTES((new_data + initial_null_bytes),
                                (table->s->reclength - initial_null_bytes));

  unsigned char *value =
      new unsigned char[(table->s->reclength - initial_null_bytes)];
  memcpy(value, (old_data + initial_null_bytes),
         (table->s->reclength - initial_null_bytes));

  memcpy(new_data + initial_null_bytes, value,
         (table->s->reclength - initial_null_bytes));

  BYTES key_bytes_old = get_primary_key(new_data);

  // Check if keys are still the same
  if (!(key_bytes_old == key_bytes_new)) {
    delete_row(new_data);
    memcpy(new_data + initial_null_bytes, new_value_bytes.value,
           (table->s->reclength - initial_null_bytes));
    write_row(new_data);
    return 0;
    // return HA_ERR_WRONG_COMMAND;  // key content is different
  }

  // Add write statement to transaction
  Transaction *txn = static_cast<Transaction *>(
      ha_thd()->get_ha_data(blockchain_hton->slot)->ha_ptr);
  std::stringstream full_table_name;
  full_table_name << "./";
  full_table_name << table->s->db.str;
  full_table_name << "/";
  full_table_name << table->s->table_name.str;

  txn->addWrite(full_table_name.str(), key_bytes_new, new_value_bytes);
  // Execute write in table cache of transaction
  txn->table_cache.at(full_table_name.str())[key_bytes_new] = new_value_bytes;
  return 0;
}

/**
  @brief
  This will delete a row. buf will contain a copy of the row to be deleted.
  The server will call this right after the current row has been called (from
  either a previous rnd_nexT() or index call).

  @details
  If you keep a pointer to the last row or can access a primary key it will
  make doing the deletion quite a bit easier. Keep in mind that the server does
  not guarantee consecutive deletions. ORDER BY clauses can be used.

  Called in sql_acl.cc and sql_udf.cc to manage internal table
  information.  Called in sql_delete.cc, sql_insert.cc, and
  sql_select.cc. In sql_select it is used for removing duplicates
  while in insert it is used for REPLACE calls.

  @see
  sql_acl.cc, sql_udf.cc, sql_delete.cc, sql_insert.cc and sql_select.cc
*/
int ha_blockchain::delete_row(const uchar *buf) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: delete_row"));
  // DBUG_TRACE;

  // Set key
  BYTES key_bytes = get_primary_key(buf);

  // DBUG_PRINT(LOG_TAG, ("DELETE_ROW: key=%s", key.c_str()));

  // Add remove statement to transaction
  Transaction *txn = static_cast<Transaction *>(
      ha_thd()->get_ha_data(blockchain_hton->slot)->ha_ptr);
  std::stringstream full_table_name;
  full_table_name << "./";
  full_table_name << table->s->db.str;
  full_table_name << "/";
  full_table_name << table->s->table_name.str;
  txn->addRemove(full_table_name.str(), key_bytes);
  // Execute remove in table cache of transaction
  txn->table_cache.at(full_table_name.str()).erase(key_bytes);
  return 0;
}

/**
  @brief
  Positions an index cursor to the index specified in the handle. Fetches the
  row if available. If the key value is null, begin at the first key of the
  index.
*/
int ha_blockchain::index_read_map(uchar *buf, const uchar *key, key_part_map,
                                  enum ha_rkey_function func) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: index_read_map"));
  return index_read(buf, key, 0, func);
}

/**
  @brief
  Used to read forward through the index.
*/
int ha_blockchain::index_next(uchar *) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: index_next"));
  int rc;
  // DBUG_TRACE;
  rc = HA_ERR_WRONG_COMMAND;
  return rc;
}

/**
  @brief
  Used to read backwards through the index.
*/
int ha_blockchain::index_prev(uchar *) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: index_prev"));
  int rc;
  // DBUG_TRACE;
  rc = HA_ERR_WRONG_COMMAND;
  return rc;
}

/**
  @brief
  index_first() asks for the first key in the index.

  @details
  Called from opt_range.cc, opt_sum.cc, sql_handler.cc, and sql_select.cc.

  @see
  opt_range.cc, opt_sum.cc, sql_handler.cc and sql_select.cc
*/
int ha_blockchain::index_first(uchar *) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: index_first"));
  int rc;
  // DBUG_TRACE;
  rc = HA_ERR_WRONG_COMMAND;
  return rc;
}

/**
  @brief
  index_last() asks for the last key in the index.

  @details
  Called from opt_range.cc, opt_sum.cc, sql_handler.cc, and sql_select.cc.

  @see
  opt_range.cc, opt_sum.cc, sql_handler.cc and sql_select.cc
*/
int ha_blockchain::index_last(uchar *) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: index_last"));
  int rc;
  // DBUG_TRACE;
  rc = HA_ERR_WRONG_COMMAND;
  return rc;
}

// BUG REPORT (please also refer to TDBT-307)
// There was an error that a WHERE-clause on the primary key column was not
// working Since we are not using real indexes, the function below calls the get
// method of the respective blockchain adapter In this get method, all entries
// of a table are scanned (full scan) until the respective key is found
// Further, the function below also provides a workaround to handle varchar PKs
// For storing varchar columns, MySQL uses the pattern described in line 638
// However, the key that we are searching for always uses the following format
// 2 Bytes at the beginning indicating the length of the key, also if only 1
// Byte would be sufficient for storing the length information n Bytes
// representing the char sequence of the key
// This difference requires a separate handling
int ha_blockchain::index_read(uchar *buf, const uchar *key, uint,
                              enum ha_rkey_function key_func) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: index_read"));
  // allocate memory for the new pointer => MySQL library for allocating memory
  // used
  uchar *key_adj = (uchar *)my_malloc(
      0, (sizeof(key) * table->key_info[table->s->primary_key].key_length),
      MYF(0));

  // copy the values of the key to the location where key_adj is pointing to
  memcpy(key_adj, key, table->key_info[table->s->primary_key].key_length);

  // determine the size of the key
  ulong key_size = 0;
  Field *key_field;
  uint16 initial_pos = 0;
  if (table->key_info != nullptr) {
    for (uint i = 0;
         i < table->key_info[table->s->primary_key].user_defined_key_parts;
         i++) {
      key_field = table->key_info[table->s->primary_key].key_part[i].field;
      key_size = key_field->pack_length();
      // If the key_part is of type varchar and has less than 255 characters,
      // then the key needs to be adjusted If the key has <= 255 chars, the
      // key_size in Byte is: number_of_chars * 4 Byte + 1 Byte (the additional
      // Byte indicates the number of chars that are used) If the key has > 255
      // chars, the key_size in Byte is: number_of_chars * 4 Byte + 2 Byte (the
      // additional 2 Byte indicate the number of chars that are used) and won't
      // need adjustment. Hence, key_size % 4 will give us either 1 or 2 and
      // therefore we know in which range the key size falls. The reason for "%
      // 4" is that MySQL uses 4 Byte for representing one char.
      if (key_field->type() == MYSQL_TYPE_VARCHAR && key_size % 4 == 1) {
        // delete the second entry in the char array by shifting the following
        // elements to the front by 1
        memcpy(key_adj + initial_pos + 1, key_adj + initial_pos + 2,
               table->key_info[table->s->primary_key].key_length - initial_pos -
                   2);
      }
      initial_pos += key_size;
    }
    key_size = initial_pos;
  }

  // get a new thread which is then used to get the name of the database that is
  // used
  THD *thd = ha_thd();
  std::string db_name = std::string(thd->db().str);
  // create string that contains a string in the format: "./db_name/table_name"
  std::string db_str = "./" + db_name + "/" + table->alias;

  // Check that exact match is required
  if (key_func != HA_READ_KEY_EXACT) {
    return HA_ERR_WRONG_COMMAND;
  }
  // check that used index uses first column or uses the primary key of the
  // table
  KEY key_used = table->key_info[active_index];
  if (key_used.key_part->field !=
          table->key_info[table->s->primary_key].key_part->field &&
      key_used.key_part->field != *(table->field)) {
    return HA_ERR_WRONG_COMMAND;
  }

  // determine the number of leading null bytes in the resulting array
  uint initial_null_bytes = table->s->null_bytes;

  // set all elements of buf (=record) to 0
  memset(buf, 0, table->s->reclength);

  // transform the key that is to be found from byte representation to hex
  // representation
  unsigned char key_hash[HASH_SIZE];
  unsigned int hash_size;
  hash_sha256(key_adj, key_size, key_hash, &hash_size);
  BYTES key_bytes = BYTES(key_hash, hash_size);

  // Get table cache of transaction
  Transaction *txn = static_cast<Transaction *>(
      ha_thd()->get_ha_data(blockchain_hton->slot)->ha_ptr);
  std::stringstream full_table_name;
  full_table_name << "./";
  full_table_name << table->s->db.str;
  full_table_name << "/";
  full_table_name << table->s->table_name.str;

  auto table_cache = txn->table_cache.at(full_table_name.str());
  auto result_it = table_cache.find(key_bytes);

  // if an element was found, then value result is non empty and further
  // processing is necessary
  if (result_it != table_cache.end()) {
    // copy the value into the buffer
    memcpy(buf + initial_null_bytes, result_it->second.value,
           result_it->second.size);
  }

  // free the memory that was used for storing the adjusted key pointer => MySQL
  // library for allocating memory used
  my_free(key_adj);

  return 0;
}

///////// Tablescan operations ////////////////////

/**
  @brief
  rnd_init() is called when the system wants the storage engine to do a table
  scan. See the example in the introduction at the top of this file to see when
  rnd_init() is called.

  @details
  Called from filesort.cc, records.cc, sql_handler.cc, sql_select.cc,
  sql_table.cc, and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc and
  sql_update.cc
*/
int ha_blockchain::rnd_init(bool) {
  // DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: rnd_init"));
  //  DBUG_TRACE;
  //  DBUG_PRINT(LOG_TAG, ("RND_INIT:"));

  // Position in result vector
  current_position = -1;

  // Get table cache of transaction
  Transaction *txn = static_cast<Transaction *>(
      ha_thd()->get_ha_data(blockchain_hton->slot)->ha_ptr);
  std::stringstream full_table_name;
  full_table_name << "./";
  full_table_name << table->s->db.str;
  full_table_name << "/";
  full_table_name << table->s->table_name.str;

  auto table_cache = txn->table_cache.at(full_table_name.str());
  for (auto entry : table_cache) {
    auto tuple = std::make_tuple(entry.first, entry.second);
    all_items.push_back(tuple);
  }

  return 0;
}

int ha_blockchain::rnd_end() {
  // DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: rnd_end"));
  //  DBUG_TRACE;
  //  DBUG_PRINT(LOG_TAG, ("RND_END:"));

  // Clear read cache
  all_items.clear();

  return 0;
}

/**
  @brief
  This is called for each row of the table scan. When you run out of records
  you should return HA_ERR_END_OF_FILE. Fill buff up with the row information.
  The Field structure for the table is the key to getting data into buf
  in a manner that will allow the server to understand it.

  @details
  Called from filesort.cc, records.cc, sql_handler.cc, sql_select.cc,
  sql_table.cc, and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc and
  sql_update.cc
*/
int ha_blockchain::rnd_next(uchar *buf) {
  // DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: rnd_next"));
  //  DBUG_TRACE;

  current_position++;  // necessary to do before fetching row so that
                       // position() still works

  // DBUG_PRINT(LOG_TAG, ("RND_NEXT: position=%llu", current_position));

  return find_current_row(buf);
}

/**
  @brief
  position() is called after each call to rnd_next() if the data needs
  to be ordered. You can do something like the following to store
  the position:
  @code
  my_store_ptr(ref, ref_length, current_position);
  @endcode

  @details
  The server uses ref to store data. ref_length in the above case is
  the size needed to store current_position. ref is just a byte array
  that the server will maintain. If you are using offsets to mark rows, then
  current_position should be the offset. If it is a primary key like in
  BDB, then it needs to be a primary key.

  Called from filesort.cc, sql_select.cc, sql_delete.cc, and sql_update.cc.

  @see
  filesort.cc, sql_select.cc, sql_delete.cc and sql_update.cc
*/
void ha_blockchain::position(const uchar *) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: position"));
  // DBUG_TRACE;
  // DBUG_PRINT(LOG_TAG, ("POSITION: %llu", current_position));
  my_store_ptr(ref, ref_length, current_position);
}

/**
  @brief
  This is like rnd_next, but you are given a position to use
  to determine the row. The position will be of the type that you stored in
  ref. You can use ha_get_ptr(pos,ref_length) to retrieve whatever key
  or position you saved when position() was called.

  @details
  Called from filesort.cc, records.cc, sql_insert.cc, sql_select.cc, and
  sql_update.cc.

  @see
  filesort.cc, records.cc, sql_insert.cc, sql_select.cc and sql_update.cc
*/
int ha_blockchain::rnd_pos(uchar *buf, uchar *pos) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: rnd_pos"));
  // DBUG_TRACE;

  auto position = my_get_ptr(pos, ref_length);

  // DBUG_PRINT(LOG_TAG, ("RND_POS: pos=%llu", position));

  return find_row(position, buf);
}

/**
  @brief
  ::info() is used to return information to the optimizer. See my_base.h for
  the complete description.

  @details
  Currently this table handler doesn't implement most of the fields really
  needed. SHOW also makes use of this data.

  You will probably want to have the following in your code:
  @code
  if (records < 2)
    records = 2;
  @endcode
  The reason is that the server will optimize for cases of only a single
  record. If, in a table scan, you don't know the number of records, it
  will probably be better to set records to two so you can return as many
  records as you need. Along with records, a few more variables you may wish
  to set are:
    records
    deleted
    data_file_length
    index_file_length
    delete_length
    check_time
  Take a look at the public variables in handler.h for more information.

  Called in filesort.cc, ha_heap.cc, item_sum.cc, opt_sum.cc, sql_delete.cc,
  sql_delete.cc, sql_derived.cc, sql_select.cc, sql_select.cc, sql_select.cc,
  sql_select.cc, sql_select.cc, sql_show.cc, sql_show.cc, sql_show.cc,
  sql_show.cc, sql_table.cc, sql_union.cc, and sql_update.cc.

  @see
  filesort.cc, ha_heap.cc, item_sum.cc, opt_sum.cc, sql_delete.cc,
  sql_delete.cc, sql_derived.cc, sql_select.cc, sql_select.cc, sql_select.cc,
  sql_select.cc, sql_select.cc, sql_show.cc, sql_show.cc, sql_show.cc,
  sql_show.cc, sql_table.cc, sql_union.cc and sql_update.cc
*/
int ha_blockchain::info(uint) {
  // DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: info"));
  //  DBUG_TRACE;
  stats.records = 10;
  return 0;
}

/**
  @brief
  extra() is called whenever the server wishes to send a hint to
  the storage engine. The myisam engine implements the most hints.
  ha_innodb.cc has the most exhaustive list of these hints.

    @see
  ha_innodb.cc
*/
int ha_blockchain::extra(enum ha_extra_function) {
  // DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: extra"));
  //  DBUG_TRACE;
  return 0;
}

/**
  @brief
  Used to delete all rows in a table, including cases of truncate and cases
  where the optimizer realizes that all rows will be removed as a result of an
  SQL statement.

  @details
  Called from item_sum.cc by Item_func_group_concat::clear(),
  Item_sum_count_distinct::clear(), and Item_func_group_concat::clear().
  Called from sql_delete.cc by mysql_delete().
  Called from sql_select.cc by JOIN::reinit().
  Called from sql_union.cc by st_select_lex_unit::exec().

  @see
  Item_func_group_concat::clear(), Item_sum_count_distinct::clear() and
  Item_func_group_concat::clear() in item_sum.cc;
  mysql_delete() in sql_delete.cc;
  JOIN::reinit() in sql_select.cc and
  st_select_lex_unit::exec() in sql_union.cc.
*/
int ha_blockchain::delete_all_rows() {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: delete_all_rows"));
  // DBUG_TRACE;
  return HA_ERR_WRONG_COMMAND;
}

/**
  @brief
  This create a lock on the table. If you are implementing a storage engine
  that can handle transacations look at ha_berkely.cc to see how you will
  want to go about doing this. Otherwise you should consider calling flock()
  here. Hint: Read the section "locking functions for mysql" in lock.cc to
  understand this.

  @details
  Called from lock.cc by lock_external() and unlock_external(). Also called
  from sql_table.cc by copy_data_between_tables().

  @see
  lock.cc by lock_external() and unlock_external() in lock.cc;
  the section "locking functions for mysql" in lock.cc;
  copy_data_between_tables() in sql_table.cc.
*/
int ha_blockchain::external_lock(THD *thd, int lock_type) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: external_lock"));
  DBUG_PRINT(LOG_TAG, ("EXTERNAL_LOCK called, lock-type=%d", lock_type));

  // Get pointer to memory of handler
  auto *ha_data = thd->get_ha_data(blockchain_hton->slot);
  // Cast it to a Transaction
  auto *txn = static_cast<Transaction *>(ha_data->ha_ptr);

  if (lock_type != F_UNLCK) {
    if (ha_data->ha_ptr == nullptr) {
      // Create new transaction
      ha_data->ha_ptr = new Transaction();
      txn = static_cast<Transaction *>(ha_data->ha_ptr);
      txn->init();
    }

    // Count locks
    txn->lock_count++;

    // Fill table cache with open table
    std::stringstream full_table_name;
    full_table_name << "./";
    full_table_name << table->s->db.str;
    full_table_name << "/";
    full_table_name << table->s->table_name.str;
    DBUG_PRINT(LOG_TAG, ("external_lock: full_table_name = %s",
                         full_table_name.str().c_str()));

    // Convert vector of tuples to map
    std::map<BYTES, BYTES> table_map_final;

    // for tables on data_chain
    if (txn->table_cache.find(full_table_name.str()) !=
        txn->table_cache.end()) {
          return 0;
    }

    // define bc_adapter_map_key for full table name
    std::string bc_adapter_map_key = full_table_name.str();
    auto it = bc_adapter_map.find(bc_adapter_map_key);

    if (it != bc_adapter_map.end()) {
      // Tablescan
      std::map<const BYTES, BYTES> table_map;
      if ( (it->second->get_all(table_map)) == -1 ) {
        // blockchain network is NOT available
        DBUG_PRINT(LOG_TAG,("external_lock: blockchain network is NOT available"));
        return 1;
      }

      for (auto entry : table_map) {
        BYTES value_bytes = BYTES(entry.second.value, entry.second.size);
        table_map_final.emplace(entry.first, value_bytes);
      }
    }

    // Add map to table cache of transaction
    txn->addTable(full_table_name.str(), table_map_final);

    // register statement transaction
    trans_register_ha(thd, false, blockchain_hton, nullptr);

    if (thd_test_options(thd, (OPTION_NOT_AUTOCOMMIT | OPTION_BEGIN))) {
      // register normal transaction
      trans_register_ha(thd, true, blockchain_hton, nullptr);
    }

  } else {
    // lock-type = unlock
  }

  return 0;
}

/**
  @brief
  The idea with handler::store_lock() is: The statement decides which locks
  should be needed for the table. For updates/deletes/inserts we get WRITE
  locks, for SELECT... we get read locks.

  @details
  Before adding the lock into the table lock handler (see thr_lock.c),
  mysqld calls store lock with the requested locks. Store lock can now
  modify a write lock to a read lock (or some other lock), ignore the
  lock (if we don't want to use MySQL table locks at all), or add locks
  for many tables (like we do when we are using a MERGE handler).

  Berkeley DB, for example, changes all WRITE locks to TL_WRITE_ALLOW_WRITE
  (which signals that we are doing WRITES, but are still allowing other
  readers and writers).

  When releasing locks, store_lock() is also called. In this case one
  usually doesn't have to do anything.

  In some exceptional cases MySQL may send a request for a TL_IGNORE;
  This means that we are requesting the same lock as last time and this
  should also be ignored. (This may happen when someone does a flush
  table when we have opened a part of the tables, in which case mysqld
  closes and reopens the tables and tries to get the same locks at last
  time). In the future we will probably try to remove this.

  Called from lock.cc by get_lock_data().

  @note
  In this method one should NEVER rely on table->in_use, it may, in fact,
  refer to a different thread! (this happens if get_lock_data() is called
  from mysql_lock_abort_for_thread() function)

  @see
  get_lock_data() in lock.cc
*/
THR_LOCK_DATA **ha_blockchain::store_lock(THD *, THR_LOCK_DATA **to,
                                          enum thr_lock_type) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: store_lock"));
  // Blockchain: ignore all locks --> global order determined by blockchain
  // (*to)->type = TL_IGNORE;
  DBUG_PRINT(LOG_TAG, ("STORE_LOCK called"));
  return to;
}

/**
  @brief
  Used to delete a table. By the time delete_table() has been called all
  opened references to this table will have been closed (and your globally
  shared references released). The variable name will just be the name of
  the table. You will need to remove any files you have created at this point.

  @details
  If you do not implement this, the default delete_table() is called from
  handler.cc and it will delete all files with the file extensions from
  handlerton::file_extensions.

  Called from handler.cc by delete_table and ha_create_table(). Only used
  during create if the table_flag HA_DROP_BEFORE_CREATE was specified for
  the storage engine.

  @see
  delete_table and ha_create_table() in handler.cc
*/
int ha_blockchain::delete_table(const char *name, const dd::Table *) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: delete_table"));
  // DBUG_TRACE;
  // DBUG_PRINT(LOG_TAG, ("DELETE_TABLE: tablename=%s", name));

  // First step
  // Only delete the table from local database, don't changed the metadata.
  // So ha_blockchain does nothing and return 0.
  // Without stub info
  (void)name;  // this variable is not yet used, since deletion of table is not
               // yet implemented
  return 0;
}

/**
  @brief
  Renames a table from one name to another via an alter table call.

  @details
  If you do not implement this, the default rename_table() is called from
  handler.cc and it will delete all files with the file extensions from
  handlerton::file_extensions.

  Called from sql_table.cc by mysql_rename_table().

  @see
  mysql_rename_table() in sql_table.cc
*/
int ha_blockchain::rename_table(const char *, const char *, const dd::Table *,
                                dd::Table *) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: rename_table"));
  // DBUG_TRACE;
  return HA_ERR_WRONG_COMMAND;
}

/**
  @brief
  Given a starting key and an ending key, estimate the number of rows that
  will exist between the two keys.

  @details
  end_key may be empty, in which case determine if start_key matches any rows.

  Called from opt_range.cc by check_quick_keys().

  @see
  check_quick_keys() in opt_range.cc
*/
ha_rows ha_blockchain::records_in_range(uint, key_range *, key_range *) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: records_in_range"));
  // DBUG_TRACE;
  return 1000;  // high number to avoid index usage
}

int ha_blockchain::start_stmt(THD *thd, thr_lock_type) {
  DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: start_stmt"));
  (void)thd;
  DBUG_PRINT(LOG_TAG, ("BCStorageEngine: Start_stmt"));
  return 0;
}

/******************
 * Helper methods *
 ******************/

int ha_blockchain::find_current_row(uchar *buf) {
  // DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: find_current_row"));
  return find_row(current_position, buf);
}

int ha_blockchain::find_row(my_off_t index, uchar *buf) {
  // DBUG_PRINT(LOG_TAG, ("ha_blockchain_method_call: find_row"));
  //  set required zero
  uint initial_null_bytes = table->s->null_bytes;
  memset(buf, 0, table->s->reclength);

  try {
    // Get value from result vector at index
    memcpy(buf + initial_null_bytes, std::get<1>(all_items.at(index)).value,
           std::get<1>(all_items.at(index)).size);

  } catch (const std::out_of_range &) {
    return HA_ERR_END_OF_FILE;
  }

  return 0;
}

// Plugin parameter
struct st_mysql_storage_engine blockchain_storage_engine = {
    MYSQL_HANDLERTON_INTERFACE_VERSION};

/********************
 * Config variables *
 *******************/

static MYSQL_SYSVAR_STR(
    bc_configuration_path, config_configuration_path,
    PLUGIN_VAR_RQCMDARG | PLUGIN_VAR_READONLY,
    "Path to BlockchainManager and BlockchainAdapter configuration folder",
    nullptr, nullptr, nullptr);

static SYS_VAR *blockchain_system_variables[] = {
    MYSQL_SYSVAR(bc_configuration_path),
    nullptr};  // config path for configurations

// Plugin descriptor
mysql_declare_plugin(blockchain){
    MYSQL_STORAGE_ENGINE_PLUGIN,
    &blockchain_storage_engine,
    "BLOCKCHAIN",
    "TU Darmstadt DM Group",
    "Blockchain storage engine",
    PLUGIN_LICENSE_GPL,
    blockchain_init_func, /* Plugin Init */
    nullptr,              /* Plugin check uninstall */
    nullptr,              /* Plugin Deinit */
    0x0001 /* 0.1 version*/,
    0,                           /* status variables */
    blockchain_system_variables, /* system variables */
    nullptr,                     /* config options */
    0,                           /* flags */
} mysql_declare_plugin_end;
