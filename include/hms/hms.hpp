#pragma once
#include "database.hpp"
#include <optional>
#include <string>
#include <vector>

namespace hms {
/**
 * @brief Patient demographic and identification entity (FR-PAT-01, FR-PAT-02).
 *
 * Represents a registered patient within the hospital management system, containing
 * their formatted public identifier (e.g., 'P000001'), full legal name, age, gender,
 * validated contact number, and ABO/Rh blood group.
 */
struct Patient {
  std::string id;         ///< Unique formatted public patient ID (e.g., "P000001").
  std::string name;       ///< Full legal name of the patient.
  int age;                ///< Patient age in years (0 to 130).
  std::string gender;     ///< Patient gender ('Male', 'Female', or 'Other').
  std::string contact;    ///< Validated contact phone number (7 to 15 digits).
  std::string bloodGroup; ///< Validated ABO/Rh blood group ('A+', 'A-', 'B+', 'B-', 'AB+', 'AB-', 'O+', 'O-').
};

/**
 * @brief Doctor professional profile entity (FR-DOC-01, FR-DOC-02).
 *
 * Represents a registered medical doctor within the hospital management system,
 * including their unique autoincrement ID, full name, clinical specialization,
 * and contact telephone number.
 */
struct Doctor {
  int id;                     ///< Unique system-assigned autoincrement doctor ID.
  std::string name;           ///< Full name of the medical practitioner.
  std::string specialization; ///< Medical specialty (e.g., "Cardiologist").
  std::string contact;        ///< Validated contact phone number.
};

/**
 * @brief Appointment consultation booking entity (FR-APP-01, FR-APP-02).
 *
 * Represents a 15-minute scheduled consultation slot between a registered patient
 * and a doctor, tracking consultation date, time, and lifecycle status
 * ('Scheduled', 'Canceled', or 'Completed').
 */
struct Appointment {
  int id;                 ///< Unique autoincrement appointment ID.
  std::string patientId;  ///< Formatted public patient ID (e.g., "P000001").
  int doctorId;           ///< ID of the assigned doctor.
  std::string date;       ///< Consultation date formatted as YYYY-MM-DD.
  std::string time;       ///< Consultation slot start time formatted as HH:MM.
  std::string status;     ///< Lifecycle status ('Scheduled', 'Canceled', or 'Completed').
};

/**
 * @brief Authenticated user session state (FR-AUTH-02).
 *
 * Maintains active user identity and authorization credentials privately within
 * the business logic layer. Stores the user's primary key, username, assigned role
 * ('Admin', 'Receptionist', or 'Doctor'), and linked doctor profile ID if applicable.
 */
struct Session {
  int userId;             ///< Database user primary key ID.
  std::string username;   ///< Authenticated username.
  std::string role;       ///< Assigned authorization role ('Admin', 'Receptionist', or 'Doctor').
  int doctorId;           ///< Associated doctor profile ID for Doctor accounts (0 for others).
};

/**
 * @brief Summary statistics of appointments for a specific calendar date (FR-REP-01).
 *
 * Aggregates appointment metrics for administrative reporting, categorizing totals
 * into overall booked, currently scheduled, canceled, and completed consultations.
 */
struct DailyReport {
  int totalBooked;  ///< Total count of appointments recorded for the date.
  int scheduled;    ///< Count of appointments currently in 'Scheduled' status.
  int canceled;     ///< Count of appointments in 'Canceled' status.
  int completed;    ///< Count of appointments in 'Completed' status.
};

/**
 * @brief Business logic layer coordinating HMS services, authentication, and validation.
 *
 * Implements core business workflows across a 3-tier layered architecture:
 * presentation calls HMS business methods, which enforce role-based access control (RBAC)
 * via private session state and execute validated operations against the local SQLite DAL.
 */
class HMS {
  Database &db_;
  std::optional<Session> session_;
  int attempts_ = 0;

  /**
   * @brief Enforces role-based access control for calling service methods (FR-AUTH-02).
   *
   * Verifies that an active session exists and that the authenticated user's role
   * is present in the list of authorized roles.
   *
   * @param roles List of authorized role names permitted to perform the operation.
   * @throws std::runtime_error If no session is active or user's role is unauthorized ("Permission denied").
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
  /**
   * @brief Constructs an HMS business service instance bound to an underlying Database.
   *
   * @param db Reference to an initialized Database instance providing SQLite persistence.
   */
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
   *
   * @return const std::optional<Session>& Reference to current active session or std::nullopt.
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

  /**
   * @brief Cancels an existing scheduled appointment (FR-APP-03).
   *
   * Enforces Receptionist-only authorization. Atomically transitions the appointment
   * record from 'Scheduled' to 'Canceled' status within a database transaction,
   * freeing the previously reserved consultation slot while preserving appointment history.
   *
   * @param appointmentId Unique identifier of the appointment to cancel.
   * @throws std::runtime_error If caller is not a Receptionist, appointment does not exist,
   *                            or appointment status is not 'Scheduled'.
   */
  void cancel(int appointmentId);

  /**
   * @brief Atomically reschedules an appointment to a new date and time slot (FR-APP-04).
   *
   * Enforces Receptionist-only authorization. Verifies that the appointment exists in
   * 'Scheduled' status, and that the doctor is available at the requested date and quarter-hour
   * time without conflict against other active bookings (excluding this appointment).
   * Atomically updates date and time within a transaction; rolls back on conflict.
   *
   * @param appointmentId Unique identifier of the appointment to reschedule.
   * @param date New date formatted as YYYY-MM-DD.
   * @param time New time slot formatted as HH:MM on a 15-minute boundary.
   * @throws std::runtime_error If caller is not a Receptionist, appointment not found,
   *                            appointment is not 'Scheduled', or doctor is unavailable / slot booked.
   * @throws std::invalid_argument If date or time formatting fails validation.
   */
  void reschedule(int appointmentId, const std::string &date,
                  const std::string &time);

  /**
   * @brief Retrieves scheduled appointments for the authenticated doctor on a given date (FR-APP-05).
   *
   * Enforces Doctor-only authorization. Returns only consultations assigned to the authenticated
   * doctor's linked doctor profile ID that are currently in 'Scheduled' status for the specified
   * date, ordered chronologically by appointment time.
   *
   * @param date Date formatted as YYYY-MM-DD.
   * @return std::vector<Appointment> List of scheduled appointments for the doctor.
   * @throws std::runtime_error If caller is not authenticated as a Doctor.
   * @throws std::invalid_argument If date formatting is invalid.
   */
  std::vector<Appointment> mySchedule(const std::string &date);

  /**
   * @brief Marks a scheduled consultation as completed (DEV-COMPLETE, FR-APP-05).
   *
   * Enforces Doctor-only authorization. Verifies that the appointment exists in 'Scheduled'
   * status and belongs specifically to the authenticated doctor's linked profile ID.
   * Transitions status to 'Completed' atomically in SQLite.
   *
   * @param appointmentId Unique identifier of the appointment to complete.
   * @throws std::runtime_error If caller is not Doctor, appointment not found, appointment
   *                            belongs to another doctor, or appointment is not 'Scheduled'.
   */
  void complete(int appointmentId);

  /**
   * @brief Generates aggregated appointment counts for a specific date (FR-REP-01).
   *
   * Enforces Administrator-only authorization. Validates the date format and queries SQLite
   * to aggregate total bookings, scheduled, canceled, and completed appointments for the day.
   *
   * @param date Date formatted as YYYY-MM-DD.
   * @return DailyReport Aggregated appointment metrics structure.
   * @throws std::runtime_error If caller is not authenticated as an Administrator.
   * @throws std::invalid_argument If date formatting is invalid.
   */
  DailyReport dailyReport(const std::string &date);

  /**
   * @brief Exports all registered patient records to a formatted file (FR-REP-02).
   *
   * Enforces Administrator-only authorization. Validates the destination file path extension
   * (.csv or .txt), formats demographic records with RFC 4180 CSV escaping or tabular plain text,
   * and writes records ordered by patient ID.
   *
   * @param path Target filesystem path ending in .csv or .txt.
   * @throws std::runtime_error If caller is not Admin, directory creation fails, or file cannot be opened.
   * @throws std::invalid_argument If path extension is neither .csv nor .txt.
   */
  void exportPatients(const std::string &path);

  /**
   * @brief Exports daily appointment summary metrics to a formatted file (FR-REP-03).
   *
   * Enforces Administrator-only authorization. Computes daily appointment statistics for
   * the specified date and exports them as either comma-separated values (.csv) or formatted
   * pipe-delimited text (.txt).
   *
   * @param date Date formatted as YYYY-MM-DD.
   * @param path Target filesystem path ending in .csv or .txt.
   * @throws std::runtime_error If caller is not Admin, date lookup fails, or file writing fails.
   * @throws std::invalid_argument If date is invalid or path extension is neither .csv nor .txt.
   */
  void exportDaily(const std::string &date, const std::string &path);
};

/**
 * @brief Validates that a text string is non-empty, within length limits, and contains printable characters.
 *
 * @param text Input text string to validate.
 * @param maxLength Maximum allowable character length.
 * @param field Field name descriptor for error messages.
 * @throws std::invalid_argument If text is empty, whitespace-only, exceeds maxLength, or contains non-printable characters.
 */
void validateText(const std::string &text, int maxLength,
                  const std::string &field);

/**
 * @brief Validates a contact phone number format.
 *
 * @param text Candidate contact number string.
 * @throws std::invalid_argument If contact string contains fewer than 7 digits, exceeds 15 characters,
 *                               or contains invalid characters other than '+', '-', space, or digits.
 */
void validateContact(const std::string &text);

/**
 * @brief Converts a 24-hour HH:MM time string to total minutes from midnight.
 *
 * @param time Time formatted as HH:MM.
 * @return int Total minutes elapsed since 00:00 (0 to 1439).
 * @throws std::invalid_argument If time format or hours/minutes are out of range.
 */
int minutes(const std::string &time);

/**
 * @brief Determines the day of the week for a given calendar date.
 *
 * @param date Date formatted as YYYY-MM-DD.
 * @return int Day of week index (0 = Sunday, 1 = Monday, ..., 6 = Saturday).
 * @throws std::invalid_argument If date format, month, or day values are invalid.
 */
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
 * @throws std::runtime_error If EVP digest calculation fails.
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
