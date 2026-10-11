#pragma once
#include <chrono>
#include <string>

#include <mysql.h>


using namespace std;

// mysql单连接封装
class Connection {
public:
  Connection();
  ~Connection();
  // 禁止拷贝：MYSQL*是独占句柄，浅拷贝会导致double-close
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;
  // 允许移动：转移句柄所有权，源对象置空避免析构时重复关闭
  Connection(Connection&& other) noexcept;
  Connection& operator=(Connection&& other) noexcept;

  bool connect(
    const string& ip,
    uint16_t port,
    const string& username,
    const string& password,
    const string& dbname
  );
  bool update(const string& sql);      // insert/update/delete
  MYSQL_RES* query(const string& sql); // select

  void refresh_alaive_time() {
    m_alive_time = chrono::steady_clock::now();
  }
  long long get_alive_time() const;

private:
  MYSQL* m_conn;
  chrono::steady_clock::time_point m_alive_time;
};
