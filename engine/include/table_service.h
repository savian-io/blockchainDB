#ifndef TABLE_SERVICE
#define TABLE_SERVICE

#include <mysql.h>
#include <string>
#include <vector>
#include "sql/table.h"

namespace trustdble {

/**
 * @brief This class provides methods to run SQL statements on the local MySQL server.
 */
class TableService{
    public:

  /**
   * @brief Execute any SQL query (INSERT, UPDATE, SELECT ...)
   *
   * @param query The query to be executed as string
   * @return 0 if successful otherwise 1
   */
  auto query(const std::string &query) -> int;

  /**
   * @brief Initialize reading the result of the previous executed query.
   *
   * @return 0 if successful otherwise 1
   */
  auto readInit() -> int;

  /**
   * @brief Reads the current row of the result set
   *
   * @return 0 if successful otherwise 1
   */
  auto readNext() -> int;

  /**
   * @brief Reads the current selected row into a string vector
   *
   * @param row Result vector to store all fields of the current row
   * @return int
   */
  auto readRow(std::vector<std::string> &row) -> int;

  /**
   * @brief Finishes reading, frees allocated memory and closes connection
   *
   * @return 0 if successful otherwise 1
   */
  auto readEnd() -> int;

 private:
  // member variables
  std::string m_username;
  std::string m_password;
  std::string m_host;
  int m_port;
  MYSQL *m_con;
  MYSQL_RES *m_result;
  MYSQL_ROW m_row;
  size_t m_num_fields;

  // HELPER METHODS //
  /**
   * @brief Initialize the client connection
   *
   * @return 0 if successful otherwise 1
   */
  auto init() -> int;
};

} // namespace table_service


#endif // TABLE_SERVICE
