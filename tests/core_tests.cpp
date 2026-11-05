#include "hms/hms.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

class TempDirectory {
public:
  std::filesystem::path path;
  TempDirectory() {
    path = std::filesystem::temp_directory_path() /
           std::filesystem::path("hms-tests-" + hms::randomSalt());
    std::filesystem::create_directory(path);
  }
  ~TempDirectory() {
    std::error_code error;
    std::filesystem::remove_all(path, error);
  }
};

static std::string readFile(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

class Core : public ::testing::Test {
protected:
  hms::Database db{":memory:"};
  hms::HMS app{db};
  void SetUp() override {
    app.bootstrap("admin", "TestPass123");
    app.login("admin", "TestPass123");
    app.addDoctor("Dr Mira Rao", "Cardiologist", "9000000001");
    app.setAvailability(1, 1, "09:00", "17:00");
    app.provision("reception", "TestPass123", "Receptionist");
    app.provision("doctor", "TestPass123", "Doctor", 1);
    app.logout();
  }
  void reception() { ASSERT_TRUE(app.login("reception", "TestPass123")); }
  hms::Patient patient() {
    return app.registerPatient("Alex Demo", 29, "Other", "9000000002", "O+");
  }
};
TEST_F(Core, TC_AUTH_01) {
  // [FR-AUTH-01, FR-AUTH-02] Verify user login, session establishment, and role assignment
  EXPECT_TRUE(app.login("admin", "TestPass123"));
  ASSERT_TRUE(app.session());
  EXPECT_EQ(app.session()->role, "Admin");
  EXPECT_EQ(app.session()->username, "admin");
  app.logout();

  // Verify Receptionist authentication and session
  EXPECT_TRUE(app.login("reception", "TestPass123"));
  ASSERT_TRUE(app.session());
  EXPECT_EQ(app.session()->role, "Receptionist");
  EXPECT_EQ(app.session()->doctorId, 0);
  app.logout();

  // Verify Doctor authentication and session with linked doctor profile
  EXPECT_TRUE(app.login("doctor", "TestPass123"));
  ASSERT_TRUE(app.session());
  EXPECT_EQ(app.session()->role, "Doctor");
  EXPECT_EQ(app.session()->doctorId, 1);
  app.logout();
}
TEST_F(Core, TC_AUTH_02) {
  // [FR-AUTH-03] Block login after three incorrect attempts within session
  EXPECT_FALSE(app.locked());
  EXPECT_FALSE(app.login("admin", "wrong-1"));
  EXPECT_FALSE(app.locked());
  EXPECT_FALSE(app.login("admin", "wrong-2"));
  EXPECT_FALSE(app.locked());
  EXPECT_FALSE(app.login("admin", "wrong-3"));
  EXPECT_TRUE(app.locked());

  // Subsequent login attempts are immediately denied once locked, even with valid credentials
  EXPECT_FALSE(app.login("admin", "TestPass123"));
}
TEST_F(Core, TC_AUTH_03) {
  // [FR-AUTH-04] Enforce session termination upon explicit user logout
  reception();
  EXPECT_TRUE(app.session().has_value());
  app.logout();
  EXPECT_FALSE(app.session().has_value());

  // All role-protected operations throw runtime_error until a new valid login occurs
  EXPECT_THROW(patient(), std::runtime_error);
  EXPECT_THROW(app.addDoctor("Dr New", "Pediatrics", "9000000099"),
               std::runtime_error);

  // Authenticating again restores authorized access
  reception();
  EXPECT_TRUE(app.session().has_value());
  EXPECT_NO_THROW(patient());
  app.logout();
}
TEST_F(Core, TC_PAT_01) {
  reception();
  auto p = patient();
  EXPECT_EQ(p.id, "P000001");
  EXPECT_EQ(p.name, "Alex Demo");
  EXPECT_EQ(p.age, 29);
  EXPECT_EQ(p.gender, "Other");
  EXPECT_EQ(p.contact, "9000000002");
  EXPECT_EQ(p.bloodGroup, "O+");
}
TEST_F(Core, TC_PAT_02) {
  // [FR-PAT-03] Search patient by exact Patient ID with Receptionist and Doctor role enforcement
  reception();
  auto p = patient();
  auto result = app.searchPatients(p.id);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].id, p.id);
  EXPECT_EQ(result[0].name, "Alex Demo");
  EXPECT_EQ(result[0].contact, "9000000002");

  // Verify Doctor role is also authorized to search patients
  app.logout();
  app.login("doctor", "TestPass123");
  auto docResult = app.searchPatients(p.id);
  ASSERT_EQ(docResult.size(), 1u);
  EXPECT_EQ(docResult[0].id, p.id);
  EXPECT_EQ(docResult[0].name, "Alex Demo");

  // Verify unauthorized roles (Admin, unauthenticated) cannot search patients
  app.logout();
  app.login("admin", "TestPass123");
  EXPECT_THROW(app.searchPatients(p.id), std::runtime_error);
  app.logout();
  EXPECT_THROW(app.searchPatients(p.id), std::runtime_error);
}
TEST_F(Core, TC_PAT_03) {
  // [FR-PAT-03] Search for non-existent patient ID returns empty result
  reception();
  EXPECT_TRUE(app.searchPatients("P999999").empty());
}
TEST_F(Core, TC_DOC_01) {
  app.login("admin", "TestPass123");
  auto d = app.addDoctor("Dr Lee", "Neurologist", "9000000003");
  EXPECT_EQ(d.id, 2);
  EXPECT_EQ(d.name, "Dr Lee");
  EXPECT_EQ(d.specialization, "Neurologist");
  EXPECT_EQ(d.contact, "9000000003");
  // Validate doctor field constraints and error handling
  EXPECT_THROW(app.addDoctor("", "Neurologist", "9000000003"), std::invalid_argument);
  EXPECT_THROW(app.addDoctor("Dr Invalid", "", "9000000003"), std::invalid_argument);
  EXPECT_THROW(app.addDoctor("Dr Invalid", "Neurologist", "123"), std::invalid_argument);
}
TEST_F(Core, TC_DOC_02) {
  reception();
  auto doctors = app.doctors("Cardiologist");
  ASSERT_EQ(doctors.size(), 1u);
  EXPECT_EQ(doctors[0].specialization, "Cardiologist");
  // Case-insensitive filtering check
  auto lower = app.doctors("cardiologist");
  ASSERT_EQ(lower.size(), 1u);
  EXPECT_EQ(lower[0].id, doctors[0].id);
  // Empty string returns all doctors
  EXPECT_FALSE(app.doctors("").empty());
  EXPECT_TRUE(app.doctors("Dentist").empty());
}
TEST_F(Core, TC_APP_01) {
  // [FR-APP-01] Book appointment with valid patient, doctor, date, and 15-min slot
  reception();
  auto a = app.book(patient().id, 1, "2026-10-19", "09:00");
  EXPECT_EQ(a.status, "Scheduled");
  EXPECT_GT(a.id, 0);
  EXPECT_EQ(a.doctorId, 1);
  EXPECT_EQ(a.date, "2026-10-19");
  EXPECT_EQ(a.time, "09:00");
}
TEST_F(Core, TC_APP_02) {
  // [FR-APP-02] Reject booking conflicts and invalid requests
  reception();
  auto p = patient();
  app.book(p.id, 1, "2026-10-19", "09:00");
  // Conflicting duplicate slot must be rejected
  EXPECT_THROW(app.book(p.id, 1, "2026-10-19", "09:00"), std::runtime_error);
  // Non-existent patient must be rejected
  EXPECT_THROW(app.book("P999999", 1, "2026-10-19", "09:15"), std::runtime_error);
  // Non-existent doctor must be rejected
  EXPECT_THROW(app.book(p.id, 999, "2026-10-19", "09:15"), std::runtime_error);
}
TEST_F(Core, TC_SEC_01) {
  // [SEC-REQ-02] Prevent SQL injection via parameterized prepared statements
  EXPECT_FALSE(app.login("' OR 1=1 --", "anything"));
  EXPECT_FALSE(app.session());
  EXPECT_FALSE(app.login("admin' --", "anything"));
  EXPECT_FALSE(app.login("' UNION SELECT 1, 'admin', 'hash', 'salt', 'Admin', 0 --", "anything"));
}
TEST_F(Core, RolesEnforcedInServices) {
  // [SEC-REQ-03, FR-AUTH-02] Enforce backend role-based authorization on service operations
  // Doctor role: blocked from patient registration, doctor creation, provisioning, and booking
  app.login("doctor", "TestPass123");
  EXPECT_THROW(patient(), std::runtime_error);
  EXPECT_THROW(app.addDoctor("No", "No", "9000000001"), std::runtime_error);
  EXPECT_THROW(app.provision("bad", "TestPass123", "Admin"),
               std::runtime_error);
  EXPECT_THROW(app.book("P000001", 1, "2026-10-19", "09:00"),
               std::runtime_error);
  EXPECT_THROW(app.cancel(1), std::runtime_error);
  EXPECT_THROW(app.dailyReport("2026-10-19"), std::runtime_error);
  app.logout();

  // Receptionist role: blocked from doctor addition, provisioning, dailyReport, and completion
  reception();
  EXPECT_THROW(app.addDoctor("Dr Denied", "Cardiology", "9000000099"),
               std::runtime_error);
  EXPECT_THROW(app.provision("user2", "TestPass123", "Doctor", 1),
               std::runtime_error);
  EXPECT_THROW(app.dailyReport("2026-10-19"), std::runtime_error);
  EXPECT_THROW(app.complete(1), std::runtime_error);
  app.logout();

  // Unauthenticated caller: blocked from all protected operations
  EXPECT_THROW(patient(), std::runtime_error);
  EXPECT_THROW(app.addDoctor("No", "No", "9000000001"), std::runtime_error);
  EXPECT_THROW(app.provision("bad", "TestPass123", "Admin"),
               std::runtime_error);
}
TEST_F(Core, HashesAreSaltedAndNeverPlaintext) {
  // [SEC-REQ-01] Passwords must never be stored in plain text.
  // Validate 128-bit CSPRNG salt (32 hex characters) and SHA-256 digest (64 hex characters).
  auto q = db.prepare("SELECT password_hash,salt FROM users ORDER BY id");
  ASSERT_TRUE(q.step());
  const auto first = q.text(0);
  EXPECT_NE(first, "TestPass123");
  EXPECT_EQ(first.size(), 64u);
  EXPECT_EQ(q.text(1).size(), 32u);
  ASSERT_TRUE(q.step());
  EXPECT_NE(first, q.text(0));
}
TEST_F(Core, BootstrapCannotBypassAdmin) {
  EXPECT_THROW(app.bootstrap("attacker", "TestPass123"), std::runtime_error);
  EXPECT_THROW(app.provision("attacker", "TestPass123", "Admin"),
               std::runtime_error);
}
TEST_F(Core, PatientValidationAndUniqueIds) {
  reception();
  EXPECT_NE(patient().id, patient().id);
  EXPECT_THROW(app.registerPatient("", 29, "Other", "9000000002", "O+"),
               std::invalid_argument);
  EXPECT_THROW(app.registerPatient("Alex", 131, "Other", "9000000002", "O+"),
               std::invalid_argument);
  EXPECT_THROW(app.registerPatient("Alex", 29, "Other", "abc", "O+"),
               std::invalid_argument);
  EXPECT_THROW(app.registerPatient("Alex", 29, "Other", "9000000002", "Q+"),
               std::invalid_argument);
  EXPECT_THROW(app.registerPatient("Alex", 29, "Invalid", "9000000002", "O+"),
               std::invalid_argument);
}
TEST_F(Core, SearchByNameAndLiteralWildcards) {
  // [FR-PAT-03] Search by name substring, case-insensitivity, and SQL LIKE wildcard escaping
  reception();
  patient();
  EXPECT_EQ(app.searchPatients("Alex").size(), 1u);
  EXPECT_EQ(app.searchPatients("alex").size(), 1u); // Case-insensitive matching
  EXPECT_EQ(app.searchPatients("Demo").size(), 1u);
  EXPECT_EQ(app.searchPatients("lex").size(), 1u);  // Substring matching
  EXPECT_TRUE(app.searchPatients("%").empty());     // Escaped wildcard: '%'
  EXPECT_TRUE(app.searchPatients("_").empty());     // Escaped wildcard: '_'
  EXPECT_TRUE(app.searchPatients("!").empty());     // Escaped escape character: '!'
  auto p = app.registerPatient("O'Neil", 20, "Male", "9000000004", "A+");
  EXPECT_EQ(app.searchPatients("O'Neil")[0].id, p.id);
  EXPECT_EQ(app.searchPatients("o'neil")[0].id, p.id);

  // Search input validation
  EXPECT_THROW(app.searchPatients(""), std::invalid_argument);
  EXPECT_THROW(app.searchPatients("   "), std::invalid_argument);
  EXPECT_THROW(app.searchPatients(std::string(101, 'a')), std::invalid_argument);
}
TEST_F(Core, AvailabilityAndCalendarValidation) {
  reception();
  auto p = patient();
  EXPECT_THROW(app.book(p.id, 1, "2026-10-20", "09:00"), std::runtime_error);
  EXPECT_THROW(app.book(p.id, 1, "2026-10-19", "17:00"), std::runtime_error);
  EXPECT_THROW(app.book(p.id, 1, "2026-10-19", "09:05"), std::runtime_error);
  EXPECT_THROW(app.book(p.id, 1, "2026-02-30", "09:00"), std::invalid_argument);
  EXPECT_NO_THROW(app.book(p.id, 1, "2026-10-19", "16:45"));
  EXPECT_EQ(hms::weekday("2024-02-29"), 4);
  EXPECT_THROW(hms::weekday("1900-02-29"), std::invalid_argument);
}
TEST_F(Core, ForeignKeysAndDoctorLinks) {
  app.login("admin", "TestPass123");
  EXPECT_THROW(app.setAvailability(999, 1, "09:00", "17:00"),
               std::runtime_error);
  EXPECT_THROW(app.provision("bad", "TestPass123", "Doctor", 999),
               std::runtime_error);
  EXPECT_THROW(app.provision("bad", "TestPass123", "Doctor"),
               std::invalid_argument);
  EXPECT_THROW(app.provision("bad", "TestPass123", "Patient"),
               std::invalid_argument);
}

TEST_F(Core, UpdateContact) {
  // [FR-PAT-04] Receptionist updates patient contact details in SQLite while preserving other details
  reception();
  auto p = patient();
  app.updateContact(p.id, "9000000008");
  auto updated = app.searchPatients(p.id);
  ASSERT_EQ(updated.size(), 1u);
  EXPECT_EQ(updated[0].id, p.id);
  EXPECT_EQ(updated[0].name, p.name);
  EXPECT_EQ(updated[0].age, p.age);
  EXPECT_EQ(updated[0].gender, p.gender);
  EXPECT_EQ(updated[0].bloodGroup, p.bloodGroup);
  EXPECT_EQ(updated[0].contact, "9000000008");

  // Error handling: non-existent patient ID and invalid contact formats
  EXPECT_THROW(app.updateContact("P999999", "9000000008"), std::runtime_error);
  EXPECT_THROW(app.updateContact(p.id, "invalid"), std::invalid_argument);
  EXPECT_THROW(app.updateContact(p.id, "123"), std::invalid_argument);

  // Role authorization: Doctor, Admin, and unauthenticated callers cannot update patient contacts
  app.logout();
  app.login("doctor", "TestPass123");
  EXPECT_THROW(app.updateContact(p.id, "9000000008"), std::runtime_error);
  app.logout();
  app.login("admin", "TestPass123");
  EXPECT_THROW(app.updateContact(p.id, "9000000008"), std::runtime_error);
  app.logout();
  EXPECT_THROW(app.updateContact(p.id, "9000000008"), std::runtime_error);
}
TEST_F(Core, TC_APP_03) {
  // [FR-APP-03] Cancel an appointment (receptionist role, slot freed, history preserved)
  reception();
  auto p = patient();
  auto a = app.book(p.id, 1, "2026-10-19", "09:00");
  app.cancel(a.id);
  auto q = db.prepare("SELECT status FROM appointments WHERE id=?");
  q.bind(1, a.id);
  ASSERT_TRUE(q.step());
  EXPECT_EQ(q.text(0), "Canceled");
  // Freed slot can now be re-booked by another patient
  EXPECT_NO_THROW(app.book(p.id, 1, "2026-10-19", "09:00"));
  // Canceling already canceled appointment must throw
  EXPECT_THROW(app.cancel(a.id), std::runtime_error);
  // Canceling non-existent appointment must throw
  EXPECT_THROW(app.cancel(999), std::runtime_error);
}
TEST_F(Core, RescheduleIsAtomic) {
  // [FR-APP-04] Reschedule an appointment with atomic rollback on conflict
  reception();
  auto p = patient();
  auto a = app.book(p.id, 1, "2026-10-19", "09:00");
  app.book(p.id, 1, "2026-10-19", "09:15");
  // Conflict: target slot already taken by another appointment
  EXPECT_THROW(app.reschedule(a.id, "2026-10-19", "09:15"), std::runtime_error);
  {
    // Atomic rollback check: original time must be preserved
    auto q = db.prepare("SELECT appt_time FROM appointments WHERE id=?");
    q.bind(1, a.id);
    ASSERT_TRUE(q.step());
    EXPECT_EQ(q.text(0), "09:00");
  }
  // Invalid calendar date format rejected
  EXPECT_THROW(app.reschedule(a.id, "2026-02-30", "09:00"),
               std::invalid_argument);
  // Valid reschedule to an open slot succeeds
  EXPECT_NO_THROW(app.reschedule(a.id, "2026-10-19", "09:30"));
  {
    auto q = db.prepare("SELECT appt_time FROM appointments WHERE id=?");
    q.bind(1, a.id);
    ASSERT_TRUE(q.step());
    EXPECT_EQ(q.text(0), "09:30");
  }
  // Original slot (09:00) is now freed and can be re-booked
  EXPECT_NO_THROW(app.book(p.id, 1, "2026-10-19", "09:00"));
  app.cancel(a.id);
  // Cannot reschedule canceled appointment
  EXPECT_THROW(app.reschedule(a.id, "2026-10-19", "09:45"), std::runtime_error);
}
TEST_F(Core, DoctorsSeeOnlyOwnSchedule) {
  // [FR-APP-05] View doctor's daily appointments (doctor isolation & chronological order)
  app.login("admin", "TestPass123");
  auto d = app.addDoctor("Dr Lee", "Dentist", "9000000003");
  app.setAvailability(d.id, 1, "09:00", "17:00");
  app.provision("doctor2", "TestPass123", "Doctor", d.id);
  app.logout();
  reception();
  auto p = patient();
  auto a = app.book(p.id, 1, "2026-10-19", "09:00");
  auto other = app.book(p.id, d.id, "2026-10-19", "09:00");
  app.logout();
  // Doctor 1 views schedule
  app.login("doctor", "TestPass123");
  auto result = app.mySchedule("2026-10-19");
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].id, a.id);
  EXPECT_EQ(result[0].patientId, p.id);
  EXPECT_EQ(result[0].time, "09:00");
  // Cannot complete another doctor's consultation
  EXPECT_THROW(app.complete(other.id), std::runtime_error);
  // Malformed date strings rejected
  EXPECT_THROW(app.mySchedule("bad"), std::invalid_argument);
  // Days with no appointments return empty vector
  EXPECT_TRUE(app.mySchedule("2026-10-20").empty());
}
TEST_F(Core, DailyCountsAndCompletion) {
  reception();
  auto p = patient();
  auto a = app.book(p.id, 1, "2026-10-19", "09:00");
  auto b = app.book(p.id, 1, "2026-10-19", "09:15");
  app.book(p.id, 1, "2026-10-19", "09:30");
  app.cancel(b.id);
  app.logout();
  app.login("doctor", "TestPass123");
  app.complete(a.id);
  EXPECT_THROW(app.complete(a.id), std::runtime_error);
  app.logout();
  app.login("admin", "TestPass123");
  auto r = app.dailyReport("2026-10-19");
  EXPECT_EQ(r.totalBooked, 3);
  EXPECT_EQ(r.scheduled, 1);
  EXPECT_EQ(r.canceled, 1);
  EXPECT_EQ(r.completed, 1);
  EXPECT_EQ(app.dailyReport("2026-10-20").totalBooked, 0);
  app.logout();
  reception();
  EXPECT_THROW(app.cancel(a.id), std::runtime_error);
}
TEST_F(Core, ReportContents) {
  reception();
  auto p =
      app.registerPatient("Demo, \"Quoted\"", 29, "Other", "9000000002", "O+");
  app.book(p.id, 1, "2026-10-19", "09:00");
  app.logout();
  app.login("admin", "TestPass123");
  const auto roster = app.patientReport();
  ASSERT_EQ(roster.size(), 1u);
  EXPECT_EQ(roster.front().id, p.id);
  EXPECT_EQ(roster.front().name, p.name);
  EXPECT_EQ(roster.front().age, p.age);
  EXPECT_EQ(roster.front().gender, p.gender);
  EXPECT_EQ(roster.front().contact, p.contact);
  EXPECT_EQ(roster.front().bloodGroup, p.bloodGroup);
  TempDirectory temp;
  app.exportPatients((temp.path / "patients.csv").string());
  app.exportPatients((temp.path / "patients.txt").string());
  app.exportDaily("2026-10-19", (temp.path / "daily.csv").string());
  app.exportDaily("2026-10-19", (temp.path / "daily.txt").string());
  EXPECT_NE(
      readFile(temp.path / "patients.csv").find("\"Demo, \"\"Quoted\"\"\""),
      std::string::npos);
  EXPECT_NE(readFile(temp.path / "patients.txt").find(p.id), std::string::npos);
  EXPECT_NE(readFile(temp.path / "daily.csv").find("2026-10-19,1,1,0,0\r\n"),
            std::string::npos);
  EXPECT_NE(
      readFile(temp.path / "daily.txt").find("2026-10-19 | 1 | 1 | 0 | 0"),
      std::string::npos);
}
TEST_F(Core, ReportErrorsAndPermissions) {
  TempDirectory temp;
  auto path = (temp.path / "patients.csv").string();
  reception();
  EXPECT_THROW(app.patientReport(), std::runtime_error);
  EXPECT_THROW(app.exportPatients(path), std::runtime_error);
  EXPECT_THROW(app.dailyReport("2026-10-19"), std::runtime_error);
  EXPECT_THROW(app.exportDaily("2026-10-19", path), std::runtime_error);
  EXPECT_FALSE(std::filesystem::exists(path));
  app.logout();
  app.login("admin", "TestPass123");
  EXPECT_THROW(app.exportPatients((temp.path / "wrong.db").string()),
               std::invalid_argument);
  std::filesystem::create_directory(temp.path / "blocked.csv");
  EXPECT_THROW(app.exportPatients((temp.path / "blocked.csv").string()),
               std::runtime_error);
}
TEST_F(Core, AvailabilityUpdates) {
  reception();
  auto p = patient();
  app.book(p.id, 1, "2026-10-19", "09:00");
  app.logout();
  app.login("admin", "TestPass123");
  EXPECT_THROW(app.setAvailability(1, 1, "10:00", "17:00"), std::runtime_error);
  EXPECT_THROW(app.setAvailability(1, 7, "09:00", "17:00"),
               std::invalid_argument);
  EXPECT_THROW(app.setAvailability(1, 1, "17:00", "09:00"),
               std::invalid_argument);
  EXPECT_NO_THROW(app.setAvailability(1, 1, "09:00", "12:00"));
}
TEST_F(Core, TC_NFR_01) {
  reception();
  {
    hms::Transaction tx(db);
    for (int i = 0; i < 10000; ++i) {
      auto q = db.prepare(
          "INSERT INTO patients(public_id,name,age,gender,contact,blood_group) "
          "VALUES(?,?,29,'Other','9000000002','O+')");
      q.bind(1, "PERF" + std::to_string(i));
      q.bind(2, "Performance Patient " + std::to_string(i));
      q.step();
    }
    tx.commit();
  }
  auto start = std::chrono::steady_clock::now();
  auto result = app.searchPatients("Performance Patient");
  auto seconds =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
          .count();
  EXPECT_EQ(result.size(), 10000u);
  EXPECT_LT(seconds, 2.0);
  RecordProperty("query_seconds", seconds);
}
TEST_F(Core, TC_NFR_02) {
  // The external Valgrind invocation supplies the leak verdict for this
  // workflow.
  reception();
  auto p = patient();
  auto a = app.book(p.id, 1, "2026-10-19", "09:00");
  app.reschedule(a.id, "2026-10-19", "09:15");
  app.cancel(a.id);
  app.logout();
  EXPECT_FALSE(app.session());
}
TEST(Recovery, MissingDatabaseAndPersistence) {
  TempDirectory temp;
  auto path = (temp.path / "nested" / "hms.db").string();
  {
    hms::Database db(path);
    hms::HMS app(db);
    app.bootstrap("admin", "TestPass123");
    app.login("admin", "TestPass123");
    app.provision("reception", "TestPass123", "Receptionist");
    app.logout();
    app.login("reception", "TestPass123");
    app.registerPatient("Persistent Patient", 45, "Male", "9000000009", "AB+");
  }
  {
    hms::Database db(path);
    hms::HMS app(db);
    ASSERT_TRUE(app.login("reception", "TestPass123"));
    auto result = app.searchPatients("P000001");
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].name, "Persistent Patient");
  }
}

// [NFR-REL-01] Verify the actual production connection, including reopen.
TEST(Reliability, DurableSettingsAndCommittedPersistence) {
  TempDirectory temp;
  const auto path = (temp.path / "reliability.db").string();
  for (int pass = 0; pass < 2; ++pass) {
    hms::Database db(path);
    {
      auto journal = db.prepare("PRAGMA journal_mode");
      ASSERT_TRUE(journal.step());
      EXPECT_EQ(journal.text(0), "wal");
      auto sync = db.prepare("PRAGMA synchronous");
      ASSERT_TRUE(sync.step());
      EXPECT_EQ(sync.integer(0), 2);
      auto fk = db.prepare("PRAGMA foreign_keys");
      ASSERT_TRUE(fk.step());
      EXPECT_EQ(fk.integer(0), 1);
    }
    if (pass == 0) {
      hms::Transaction tx(db);
      db.execute("INSERT INTO doctors(name,specialization,contact) VALUES('Persistent','General','9000000001')");
      tx.commit();
    }
    auto rows = db.prepare("SELECT count(*) FROM doctors");
    ASSERT_TRUE(rows.step());
    EXPECT_EQ(rows.integer(0), 1);
    auto integrity = db.prepare("PRAGMA integrity_check");
    ASSERT_TRUE(integrity.step());
    EXPECT_EQ(integrity.text(0), "ok");
  }
}

TEST(Reliability, ScopeExitAndExceptionRollBackAllWrites) {
  hms::Database db(":memory:");
  for (bool unwind : {false, true}) {
    try {
      hms::Transaction tx(db);
      db.execute("INSERT INTO doctors(name,specialization,contact) VALUES('Pending','General','9000000001')");
      if (unwind)
        db.execute("INSERT INTO availability VALUES(999,1,540,600)");
    } catch (const std::runtime_error &) {
      EXPECT_TRUE(unwind);
    }
    auto rows = db.prepare("SELECT count(*) FROM doctors");
    ASSERT_TRUE(rows.step());
    EXPECT_EQ(rows.integer(0), 0);
  }
  hms::Transaction next(db);
  EXPECT_NO_THROW(next.commit());
}

TEST(Reliability, FailedCommitKeepsRollbackArmed) {
  hms::Database db(":memory:");
  db.execute("CREATE TABLE deferred_link(doctor_id INTEGER REFERENCES doctors(id) DEFERRABLE INITIALLY DEFERRED)");
  {
    hms::Transaction tx(db);
    db.execute("INSERT INTO deferred_link VALUES(999)");
    EXPECT_THROW(tx.commit(), std::runtime_error);
  }
  auto rows = db.prepare("SELECT count(*) FROM deferred_link");
  ASSERT_TRUE(rows.step());
  EXPECT_EQ(rows.integer(0), 0);
  hms::Transaction next(db);
  EXPECT_NO_THROW(next.commit());
}
