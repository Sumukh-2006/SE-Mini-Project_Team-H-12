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
/** Counts appointments by current date. Total booked includes all three statuses. */
struct DailyReport {
  int totalBooked, scheduled, canceled, completed;
};

/** Business layer. Session state is private; every sensitive operation checks
 * its role. */
class HMS {
  Database &db_;
  std::optional<Session> session_;
  int attempts_ = 0;
  /**
   * @brief Enforces backend role-based authorization before sensitive service execution (SEC-REQ-03, FR-AUTH-02).
   *
   * Verifies that an authenticated session is active and the current user's role
   * is present in the specified allowed roles list. Rejects unauthorized callers at the
   * backend C++ service layer prior to executing database queries or mutating state.
   *
   * @param roles List of authorized role names (e.g., {"Admin"}, {"Receptionist"}, {"Doctor"}).
   * @throws std::runtime_error With message "Permission denied" if session is absent or role is unauthorized.
   */
  void require(const std::vector<std::string> &roles) const;
  /**
   * @brief Validates doctor working hours and checks for conflicting bookings (FR-APP-02).
   *
   * Verifies quarter-hour boundary alignment, doctor weekly shift hours, and absence
   * of other active Scheduled appointments.
   *
   * @param doctorId Registered doctor ID.
   * @param date Date formatted as YYYY-MM-DD.
   * @param time Time formatted as HH:MM.
   * @param exclude Optional appointment ID to exclude (used during rescheduling).
   * @return true if slot is available; false otherwise.
   */
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
   * @brief Authenticates a user with constant-time hash verification and brute-force lockout (FR-AUTH-01, FR-AUTH-03, SEC-REQ-01).
   *
   * Queries stored credentials for the username, computes the salted SHA-256 hash using
   * the stored 128-bit salt, and performs constant-time comparison (CRYPTO_memcmp)
   * to protect against timing-attack leakage.
   *
   * On successful authentication, initializes private session state and resets the failed
   * attempt counter (attempts_ = 0).
   * On failed authentication, increments attempts_ (++attempts_). If 3 consecutive failures
   * occur, locked() becomes true and subsequent login attempts are denied immediately.
   *
   * @param username Username attempting to log in.
   * @param password Candidate plaintext password.
   * @return true If authentication succeeds and session is initialized.
   * @return false If credentials do not match or authentication is locked.
   * @throws std::runtime_error If a session is already active (must log out first).
   */
  bool login(const std::string &username, const std::string &password);

  /**
   * @brief Terminates the active user session and revokes authenticated privileges (FR-AUTH-04).
   *
   * Clears in-memory session credentials via session_.reset(), ensuring that any subsequent
   * role-protected business operation throws std::runtime_error ("Permission denied") until
   * a valid user authenticates again.
   */
  void logout();

  /**
   * @brief Enforces account lockout after three failed login attempts (FR-AUTH-03).
   *
   * Tracks consecutive unsuccessful authentication attempts within the active process
   * attempt session. When attempts_ reaches or exceeds the threshold of 3 (attempts_ >= 3),
   * access is denied, subsequent authentication calls are immediately blocked, and the
   * console application terminates with access denied exit status 2.
   *
   * @return true If 3 or more consecutive failed login attempts have occurred; false otherwise.
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
  /**
   * @brief Searches for patients by exact Patient ID or name substring (FR-PAT-03).
   *
   * Enforces role-based access control allowing only Receptionist and Doctor roles.
   * Performs an exact lookup by public Patient ID (e.g., 'P000001') or a case-insensitive
   * substring search on the patient's name. SQL LIKE wildcard characters ('%', '_', '!')
   * in user input are escaped using '!' with ESCAPE '!' to prevent wildcard injection
   * and unauthorized database scraping.
   *
   * @param query Search query matching exact Patient ID or name substring (up to 100 characters).
   * @return std::vector<Patient> Matching patient records ordered by id.
   * @throws std::runtime_error If caller is unauthenticated or not authorized (neither Receptionist nor Doctor).
   * @throws std::invalid_argument If query is invalid (empty, all-whitespace, non-printable, or > 100 characters).
   */
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
  /**
   * @brief Configures weekly working hours for a registered doctor (FR-DOC-03).
   *
   * Enforces Admin-only authorization. Validates that the weekday is in range 0-6 (0=Sunday),
   * start and end times are aligned to 15-minute boundaries (quarter hours), and start < end.
   * Atomically checks that existing 'Scheduled' appointments on that weekday are not excluded
   * by the new working window, rejecting the update if a conflict would occur.
   *
   * @param doctorId Registered doctor ID.
   * @param weekday Day of the week (0 Sunday, 1 Monday ... 6 Saturday).
   * @param start Shift start time in HH:MM format (e.g. "09:00").
   * @param end Shift end time in HH:MM format (e.g. "17:00").
   * @throws std::runtime_error If caller is not Admin, doctor does not exist, or an existing appointment would be excluded.
   * @throws std::invalid_argument If weekday or time boundary format validation fails.
   */
  void setAvailability(int doctorId, int weekday, const std::string &start,
                       const std::string &end);

  /**
   * @brief Retrieves registered doctors, optionally filtered by specialization (FR-DOC-04).
   *
   * Accessible to Administrators and Receptionists. If specialization is empty, returns all
   * registered doctors. Otherwise, performs a case-insensitive match against the specialization.
   *
   * @param specialization Medical specialization filter (empty string for all doctors).
   * @return std::vector<Doctor> List of matching doctor records ordered by ID.
   * @throws std::runtime_error If caller is not authenticated as Admin or Receptionist.
   */
  std::vector<Doctor> doctors(const std::string &specialization);
  /**
   * @brief Books an appointment slot for a patient with availability and conflict validation (FR-APP-01, FR-APP-02).
   *
   * Enforces Receptionist-only role authorization. Resolves the patient public ID,
   * validates date (YYYY-MM-DD) and 15-minute slot boundary (HH:MM), verifies doctor shift availability,
   * and ensures no conflicting active appointment exists. Inserts the record with 'Scheduled' status inside
   * an atomic SQLite transaction.
   *
   * @param patientId Formatted public patient ID (e.g., 'P000001').
   * @param doctorId Registered doctor ID.
   * @param date Date formatted as YYYY-MM-DD.
   * @param time Time formatted as HH:MM on a 15-minute boundary.
   * @return Appointment Created appointment record with assigned ID and 'Scheduled' status.
   * @throws std::runtime_error If caller is not Receptionist, patient not found, or doctor unavailable / slot booked.
   * @throws std::invalid_argument If date or time formatting fails validation.
   */
  Appointment book(const std::string &patientId, int doctorId,
                   const std::string &date, const std::string &time);
  /**
   * @brief Updates contact information for a registered patient (FR-PAT-04).
   *
   * Enforces Receptionist-only authorization. Validates the new contact number
   * (must contain 7 to 15 digits with optional +, -, or spaces). Verifies that
   * the target patient exists via their public ID and updates the contact
   * atomically in SQLite while preserving all other patient fields.
   *
   * @param patientId Formatted public patient ID (e.g., 'P000001').
   * @param contact New contact phone number (7 to 15 digits).
   * @throws std::runtime_error If caller is not authenticated as a Receptionist or patient does not exist.
   * @throws std::invalid_argument If the contact number fails format/length validation.
   */
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
  /** @brief Admin-only counts for a validated YYYY-MM-DD appointment date. */
  DailyReport dailyReport(const std::string &date);
  /** @brief Admin-only roster of all registered patients in registration order. */
  std::vector<Patient> patientReport();
  /** @brief Admin-only roster export to .txt or RFC 4180 CSV. */
  void exportPatients(const std::string &path);
  /** @brief Admin-only daily totals export to .txt or RFC 4180 CSV. */
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
