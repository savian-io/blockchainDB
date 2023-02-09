#ifndef GENERAL_HELPERS_H
#define GENERAL_HELPERS_H

#include <atomic>
#include <string>

/**
 * @brief Struct for use a shared memory, which used just a boolean
 *
 * @param signal_db_creat_ready signal if ready for create a database. true:
 * ready, false: not ready
 */
struct SHARED_MEMORY {
  std::atomic<bool> signal_db_creat_ready;
};

/**
 * @brief Method for try and set the boolean value of the shared memory
 *
 * @return 1 if set successful, 0 if not successful, -1 error
 */
auto try_and_set_SHM(bool init) -> int;

/**
 * @brief Method for reset the boolean value=true from the shared memory
 *
 */
auto resetSHM() -> bool;

/**
 * @brief Parse parameter value from query string using it's key
 * (parameter name) to find it
 *
 * @param[in] parameter_key Parameter key
 * @param[in] query_string Query string
 * @param[in] delimitter Delimitter
 *
 * @return Parsed parameter value
 */
std::string parse_keyvalue_parameter(const std::string& parameter_key,
                                     const std::string& query_string,
                                     const std::string& delimitter);

#endif // GENERAL_HELPERS_H
