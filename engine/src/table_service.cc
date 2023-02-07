#include "blockchain/table_service.h"
#include "sql/mysqld.h"

using namespace trustdble;

auto TableService::query(const std::string &query) -> int {

  int error=0;
  init();

  m_con = mysql_init(NULL);
  if (m_con == NULL) {
    fprintf(stderr, "%s\n", mysql_error(m_con));
    return 1;
  }
  if (mysql_real_connect(m_con, m_host.c_str(), m_username.c_str(),
                         m_password.c_str(), NULL, m_port, NULL,
                         0) == NULL) {
    fprintf(stderr, "%s\n", mysql_error(m_con));
    mysql_close(m_con);
    return 1;
  }

  error=mysql_real_query(m_con, query.c_str(), query.size());

  m_result = mysql_store_result(m_con);
  if (m_result == NULL) {
    fprintf(stderr, "%s\n", mysql_error(m_con));
    mysql_close(m_con);
    return 1;
  }

  m_num_fields = mysql_num_fields(m_result);

  mysql_close(m_con);

  return error;
}
// Helper Methods
auto TableService::init() -> int {
  m_username = "root";
  m_password = "";
  m_host = "localhost";
  m_port = mysqld_port;
  return 0;
}

auto TableService::readInit() -> int {

  return 0;
}

auto TableService::readNext() -> int {
  if (m_result == NULL){
    m_row=NULL;
    return 1;
  }
  if ((m_row=mysql_fetch_row(m_result)))
    return 0;
  else
    return 1;
}

auto TableService::readEnd() -> int {
  if (m_result == NULL) return 1;
  mysql_free_result(m_result);
  return 0;
}

auto TableService::readRow(std::vector<std::string> &row) -> int {
  if(!m_row)
    return 0;
  for (size_t i = 0; i < m_num_fields; i++) {
    row.push_back(std::string(m_row[i]));
  }
  return 0;
}
