#include "hms/database.hpp"
#include <filesystem>
#include <stdexcept>

namespace hms {
Statement::Statement(sqlite3 *db, const std::string &sql) {
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK)
    throw std::runtime_error(sqlite3_errmsg(db));
}
Statement::~Statement() { sqlite3_finalize(stmt_); }
void Statement::bind(int i, const std::string &value) {
  if (sqlite3_bind_text(stmt_, i, value.c_str(), static_cast<int>(value.size()),
                        SQLITE_TRANSIENT) != SQLITE_OK)
    throw std::runtime_error("Cannot bind text");
}
void Statement::bind(int i, int value) {
  if (sqlite3_bind_int(stmt_, i, value) != SQLITE_OK)
    throw std::runtime_error("Cannot bind number");
}
bool Statement::step() {
  const int rc = sqlite3_step(stmt_);
  if (rc == SQLITE_ROW)
    return true;
  if (rc == SQLITE_DONE)
    return false;
  throw std::runtime_error(sqlite3_errmsg(sqlite3_db_handle(stmt_)));
}
std::string Statement::text(int col) const {
  const auto *t = sqlite3_column_text(stmt_, col);
  return t ? reinterpret_cast<const char *>(t) : "";
}
int Statement::integer(int col) const { return sqlite3_column_int(stmt_, col); }
Database::Database(const std::string &path) {
  if (path != ":memory:") {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty())
      std::filesystem::create_directories(parent);
  }
  if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
    const std::string error =
        db_ ? sqlite3_errmsg(db_) : "Cannot open database";
    sqlite3_close(db_);
    db_ = nullptr;
    throw std::runtime_error(error);
  }
  try {
    sqlite3_busy_timeout(db_, 5000);
    execute("PRAGMA foreign_keys=ON");
    execute("PRAGMA journal_mode=WAL");
    execute("PRAGMA synchronous=FULL");
    execute(R"SQL(
CREATE TABLE IF NOT EXISTS doctors(id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT NOT NULL,specialization TEXT NOT NULL,contact TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS users(id INTEGER PRIMARY KEY AUTOINCREMENT,username TEXT NOT NULL UNIQUE,password_hash TEXT NOT NULL,salt TEXT NOT NULL,role TEXT NOT NULL CHECK(role IN ('Admin','Receptionist','Doctor')),doctor_id INTEGER UNIQUE REFERENCES doctors(id),CHECK((role='Doctor' AND doctor_id IS NOT NULL) OR (role!='Doctor' AND doctor_id IS NULL)));
CREATE TABLE IF NOT EXISTS patients(id INTEGER PRIMARY KEY AUTOINCREMENT,public_id TEXT UNIQUE,name TEXT NOT NULL,age INTEGER NOT NULL CHECK(age BETWEEN 0 AND 130),gender TEXT NOT NULL,contact TEXT NOT NULL,blood_group TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS availability(doctor_id INTEGER NOT NULL REFERENCES doctors(id),weekday INTEGER NOT NULL CHECK(weekday BETWEEN 0 AND 6),start_min INTEGER NOT NULL,end_min INTEGER NOT NULL,CHECK(start_min>=0 AND end_min<=1440 AND start_min<end_min),PRIMARY KEY(doctor_id,weekday));
CREATE TABLE IF NOT EXISTS appointments(id INTEGER PRIMARY KEY AUTOINCREMENT,patient_id INTEGER NOT NULL REFERENCES patients(id),doctor_id INTEGER NOT NULL REFERENCES doctors(id),appt_date TEXT NOT NULL,appt_time TEXT NOT NULL,status TEXT NOT NULL CHECK(status IN ('Scheduled','Canceled','Completed')));
CREATE UNIQUE INDEX IF NOT EXISTS active_slot ON appointments(doctor_id,appt_date,appt_time) WHERE status!='Canceled';
CREATE INDEX IF NOT EXISTS patient_name ON patients(name);
CREATE INDEX IF NOT EXISTS doctor_specialization ON doctors(specialization);
CREATE INDEX IF NOT EXISTS appointment_date ON appointments(appt_date);
)SQL");
  } catch (...) {
    sqlite3_close(db_);
    db_ = nullptr;
    throw;
  }
}
Database::~Database() { sqlite3_close(db_); }
Statement Database::prepare(const std::string &sql) {
  return Statement(db_, sql);
}
void Database::execute(const std::string &sql) {
  char *error = nullptr;
  if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
    const std::string message = error ? error : "Database error";
    sqlite3_free(error);
    throw std::runtime_error(message);
  }
}
int Database::lastId() const {
  return static_cast<int>(sqlite3_last_insert_rowid(db_));
}
Transaction::Transaction(Database &db) : db_(db) {
  db_.execute("BEGIN IMMEDIATE");
}
Transaction::~Transaction() {
  if (!committed_) {
    try {
      db_.execute("ROLLBACK");
    } catch (...) {
    }
  }
}
void Transaction::commit() {
  db_.execute("COMMIT");
  committed_ = true;
}
} // namespace hms
