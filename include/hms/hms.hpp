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
struct Session {
  int userId;
  std::string username, role;
  int doctorId;
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
  /** Allows one initial administrator only when no users exist. */
  void bootstrap(const std::string &username, const std::string &password);
  /** Administrator-only provisioning; Doctor accounts must link to a registered
   * doctor. */
  void provision(const std::string &username, const std::string &password,
                 const std::string &role, int doctorId = 0);
  bool login(const std::string &username, const std::string &password);
  void logout();
  bool locked() const { return attempts_ >= 3; }
  const std::optional<Session> &session() const { return session_; }
  Patient registerPatient(const std::string &name, int age,
                          const std::string &gender, const std::string &contact,
                          const std::string &blood);
  std::vector<Patient> searchPatients(const std::string &query);
  Doctor addDoctor(const std::string &name, const std::string &specialization,
                   const std::string &contact);
  void setAvailability(int doctorId, int weekday, const std::string &start,
                       const std::string &end);
  std::vector<Doctor> doctors(const std::string &specialization);
  Appointment book(const std::string &patientId, int doctorId,
                   const std::string &date, const std::string &time);
};
/** Shared validation runs in the business layer as well as interactive input
 * handling. */
void validateText(const std::string &text, int maxLength,
                  const std::string &field);
void validateContact(const std::string &text);
int minutes(const std::string &time);
int weekday(const std::string &date);
std::string passwordHash(const std::string &salt, const std::string &password);
std::string randomSalt();
} // namespace hms
