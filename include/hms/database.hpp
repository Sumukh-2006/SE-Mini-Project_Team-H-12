#pragma once
#include <sqlite3.h>
#include <string>

namespace hms {
/** Owns a prepared statement. All user input must be bound as parameters. */
class Statement {
  sqlite3_stmt *stmt_ = nullptr;

public:
  Statement(sqlite3 *db, const std::string &sql);
  ~Statement();
  Statement(const Statement &) = delete;
  Statement &operator=(const Statement &) = delete;
  void bind(int index, const std::string &value);
  void bind(int index, int value);
  bool step();
  std::string text(int column) const;
  int integer(int column) const;
};
/** Owns a SQLite connection with foreign keys, full synchronization and schema
 * initialization. */
class Database {
  sqlite3 *db_ = nullptr;

public:
  explicit Database(const std::string &path);
  ~Database();
  Database(const Database &) = delete;
  Database &operator=(const Database &) = delete;
  Statement prepare(const std::string &sql);
  void execute(const std::string &sql);
  int lastId() const;
};
/** Rolls back automatically unless commit succeeds. */
class Transaction {
  Database &db_;
  bool committed_ = false;

public:
  explicit Transaction(Database &db);
  ~Transaction();
  void commit();
};
} // namespace hms
