#pragma once
#include "database.hpp"
#include <optional>
#include <string>
#include <vector>

namespace hms {
struct Patient {
  std::string id, name;
  int age;
  std::string gender, contact, bloodGroup;
};
struct Doctor {
  int id;
  std::string name, specialization, contact;
};
struct Appointment {
  int id;
  std::string patientId;
  int doctorId;
  std::string date, time, status;
};
/**
 * @brief Authenticated user session state (FR-AUTH-02).
 *
 * Maintains active user identity and authorization credentials privately within
 * the business logic layer. Stores the user's primary key, username, assigned role
 * ('Admin', 'Receptionist', or 'Doctor'), and linked doctor profile ID if applicable.
 */
struct Session {
  int userId;
  std::string username, role;
  int doctorId;
};
/** Counts appointments by their current scheduled date and current status. */
struct DailyReport {
  int totalBooked, scheduled, canceled, completed;
};

/** Business layer. Session state is private; every sensitive operation checks
 * its role. */
class HMS {
  Database &db_;
  std::optional<Session> session_;
  int attempts_ = 0;
  void require(const std::vector<std::string> &roles) const;
  bool available(int doctorId, const std::string &date, const std::string &time,
                 int exclude = 0);

public:
  explicit HMS(Database &db) : db_(db) {}
  /**
   * @brief Bootstraps the initial system administrator account (SEC-REQ-01, FR-AUTH-02).
   *
   * Allows creation of the first administrator account only when no users exist in
   * the database. Generates a 128-bit CSPRNG salt, computes salted SHA-256 password
   * hash, and provisions the initial Admin role.
   *
   * @param username Administrator username (up to 50 printable characters).
   * @param password Administrator password (8 to 128 printable characters).
   * @throws std::runtime_error If an administrator has already been initialized.
   * @throws std::invalid_argument If username or password fails format/length constraints.
   */
  void bootstrap(const std::string &username, const std::string &password);

  /**
   * @brief Provisions a new user account with role-based access control (FR-AUTH-02, SEC-REQ-01).
   *
   * Restricted to authenticated Administrators. Enforces valid role assignment
   * ('Admin', 'Receptionist', 'Doctor') and verifies mandatory doctor profile linkage
   * for Doctor accounts. Computes salted SHA-256 credentials with unique 128-bit CSPRNG salt.
   *
   * @param username Unique username for the account.
   * @param password Plaintext password to hash and store securely.
   * @param role User role: 'Admin', 'Receptionist', or 'Doctor'.
   * @param doctorId Registered doctor ID when role is 'Doctor'; 0 otherwise.
   * @throws std::runtime_error If caller is not authenticated as an Admin.
   * @throws std::invalid_argument If credentials, role, or doctor linkage validation fails.
   */
  void provision(const std::string &username, const std::string &password,
                 const std::string &role, int doctorId = 0);

  /**
   * @brief Authenticates a user using constant-time hash verification (FR-AUTH-01, SEC-REQ-01).
   *
   * Queries stored credentials for the username, computes the salted SHA-256 hash using
   * the stored 128-bit salt, and performs constant-time comparison (CRYPTO_memcmp)
   * to protect against timing-attack leakage. Initializes private session state on success
   * and enforces lockout after 3 consecutive failed attempts.
   *
   * @param username Username attempting to log in.
   * @param password Candidate plaintext password.
   * @return true If authentication succeeds and session is initialized.
   * @return false If credentials do not match or authentication is locked.
   * @throws std::runtime_error If a session is already active (must log out first).
   */
  bool login(const std::string &username, const std::string &password);

  /**
   * @brief Terminates the active user session and clears session state (FR-AUTH-01).
   */
  void logout();

  /**
   * @brief Indicates whether authentication is locked after excessive failed attempts (FR-AUTH-01).
   * @return true If 3 or more consecutive failed login attempts have occurred.
   */
  bool locked() const { return attempts_ >= 3; }

  /**
   * @brief Retrieves the active session state if authenticated (FR-AUTH-02).
   * @return const std::optional<Session>& Current active session.
   */
  const std::optional<Session> &session() const { return session_; }
  /**
   * @brief Registers a new patient into the system (FR-PAT-01, FR-PAT-02).
   *
   * Enforces Receptionist-only authorization, validates input parameters
   * (name <= 100 chars, age between 0 and 130, gender in Male/Female/Other,
   * contact format 7-15 digits with optional symbols, valid blood group), inserts
   * into the database, generates a formatted unique alphanumeric public ID
   * (P followed by 6 zero-padded digits, e.g., P000001) within an atomic SQLite
   * transaction, and returns the populated Patient entity.
   *
   * @param name Patient full name (max 100 characters).
   * @param age Patient age (0 to 130 inclusive).
   * @param gender Patient gender ('Male', 'Female', or 'Other').
   * @param contact Patient contact number (7 to 15 digits).
   * @param blood Patient blood group ('A+', 'A-', 'B+', 'B-', 'AB+', 'AB-', 'O+', 'O-').
   * @return Patient Populated Patient struct with formatted public ID.
   * @throws std::runtime_error If caller is not authenticated as Receptionist.
   * @throws std::invalid_argument If any parameter fails validation.
   */
  Patient registerPatient(const std::string &name, int age,
                          const std::string &gender, const std::string &contact,
                          const std::string &blood);
  std::vector<Patient> searchPatients(const std::string &query);
  /**
   * @brief Registers a new doctor profile into the system (FR-DOC-01, FR-DOC-02).
   *
   * Enforces Admin-only authorization, validates parameter bounds (name <= 100 chars,
   * specialization <= 50 chars, valid contact format), inserts into the database
   * using parameterized prepared statements, and returns the assigned autoincrement ID.
   *
   * @param name Doctor's full name.
   * @param specialization Medical specialization (e.g., Cardiologist).
   * @param contact Phone contact (7-15 digits with optional +, -, spaces).
   * @return Doctor Populated Doctor struct with generated unique ID.
   * @throws std::runtime_error If caller is not authenticated as Admin.
   * @throws std::invalid_argument If name, specialization, or contact validation fails.
   */
  Doctor addDoctor(const std::string &name, const std::string &specialization,
                   const std::string &contact);
  void setAvailability(int doctorId, int weekday, const std::string &start,
                       const std::string &end);
  std::vector<Doctor> doctors(const std::string &specialization);
  Appointment book(const std::string &patientId, int doctorId,
                   const std::string &date, const std::string &time);
  /** Receptionist-only patient contact update. */
  void updateContact(const std::string &patientId, const std::string &contact);
  /** Receptionist-only cancellation and atomic rescheduling of Scheduled
   * records. */
  void cancel(int appointmentId);
  void reschedule(int appointmentId, const std::string &date,
                  const std::string &time);
  /** Returns only the authenticated doctor's Scheduled appointments for the
   * date. */
  std::vector<Appointment> mySchedule(const std::string &date);
  /** Linked doctor may complete only their own Scheduled appointment. */
  void complete(int appointmentId);
  /** Administrator-only daily totals and local CSV/text report exports. */
  DailyReport dailyReport(const std::string &date);
  void exportPatients(const std::string &path);
  void exportDaily(const std::string &date, const std::string &path);
};
/** Shared validation runs in the business layer as well as interactive input
 * handling. */
void validateText(const std::string &text, int maxLength,
                  const std::string &field);
void validateContact(const std::string &text);
int minutes(const std::string &time);
int weekday(const std::string &date);
/**
 * @brief Computes a salted SHA-256 cryptographic digest of a password (SEC-REQ-01).
 *
 * Concatenates salt and password formatted as "<salt>:<password>" and hashes using
 * OpenSSL EVP_sha256() to guarantee one-way collision-resistant credential storage.
 *
 * @param salt 32-character hexadecimal 128-bit random salt.
 * @param password The plaintext password to hash.
 * @return std::string 64-character lowercase hexadecimal SHA-256 digest.
 */
std::string passwordHash(const std::string &salt, const std::string &password);

/**
 * @brief Generates a cryptographically secure 128-bit random salt (SEC-REQ-01).
 *
 * Uses OpenSSL CSPRNG (RAND_bytes) to produce 16 cryptographically secure random bytes,
 * formatted as a 32-character lowercase hexadecimal string.
 *
 * @return std::string Hex-encoded 128-bit salt string.
 * @throws std::runtime_error If OpenSSL secure randomness is unavailable.
 */
std::string randomSalt();
} // namespace hms
