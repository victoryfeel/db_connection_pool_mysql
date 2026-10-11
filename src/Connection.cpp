#include "Connection.h"

#include <iostream>


Connection::Connection() {
  m_conn = mysql_init(nullptr);
}
Connection::~Connection() {
  if (m_conn != nullptr)
    mysql_close(m_conn);
}
Connection::Connection(Connection&& other) noexcept
    : m_conn(other.m_conn), m_alive_time(other.m_alive_time) {
  other.m_conn = nullptr;
}
Connection& Connection::operator=(Connection&& other) noexcept {
  if (this != &other) {
    if (m_conn)
      mysql_close(m_conn);
    m_conn = other.m_conn;
    m_alive_time = other.m_alive_time;
    other.m_conn = nullptr;
  }
  return *this;
}


bool Connection::connect(
  const string& ip,
  uint16_t port,
  const string& username,
  const string& password,
  const string& dbname
) {
  MYSQL* p = mysql_real_connect(
    m_conn, ip.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0
  );
  if (p == nullptr) {
    std::cerr << "[Connection] connect failed: " << mysql_error(m_conn) << std::endl;
    return false;
  }
  // 自动重连，避免长连接被服务端断开后仍标记为可用
  bool reconnect = true;
  mysql_options(m_conn, MYSQL_OPT_RECONNECT, &reconnect);
  return true;
}


bool Connection::update(const string& sql) {
  // insert、delete、update
  if (mysql_query(m_conn, sql.c_str())) {
    std::cerr << "[Connection] update failed: " << mysql_error(m_conn) << " sql=" << sql
              << std::endl;
    return false;
  }
  return true;
}

MYSQL_RES* Connection::query(const string& sql) {
  // select
  if (mysql_query(m_conn, sql.c_str())) {
    std::cerr << "[Connection] query failed: " << mysql_error(m_conn) << " sql=" << sql
              << std::endl;
    return nullptr;
  }
  return mysql_use_result(m_conn);
}

long long Connection::get_alive_time() const {
  auto now = chrono::steady_clock::now();
  auto cost = chrono::duration_cast<chrono::milliseconds>(now - m_alive_time).count();
  return cost;
}
