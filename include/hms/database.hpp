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
/**
 * @brief Owns a SQLite connection and initializes its schema.
 * File databases require WAL journaling and synchronous=FULL; foreign keys
 * enforce relational consistency. FULL requests durable commits from SQLite
 * (subject to the filesystem and storage honoring synchronization requests).
 * In-memory databases use SQLite's memory journal and have no crash durability.
 */
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
/**
 * @brief Non-copyable RAII owner of a BEGIN IMMEDIATE transaction.
 * SQLite supplies atomicity and writer isolation; constraints and business
 * validation supply consistency. commit() marks success only after COMMIT
 * succeeds. Scope exit rolls back pending writes, including exception unwinding.
 * SIGKILL/power loss cannot run destructors: SQLite WAL recovery ignores
 * uncommitted frames and preserves fully synchronized committed transactions.
 * The Database must outlive this guard; nested transactions are unsupported.
 */
class Transaction {
  Database &db_;
  bool committed_ = false;

public:
  explicit Transaction(Database &db);
  ~Transaction() noexcept;
  Transaction(const Transaction &) = delete;
  Transaction &operator=(const Transaction &) = delete;
  void commit();
};
} // namespace hms
