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
  std::ifstream in(path);
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
  EXPECT_TRUE(app.login("admin", "TestPass123"));
  ASSERT_TRUE(app.session());
  EXPECT_EQ(app.session()->role, "Admin");
}
TEST_F(Core, TC_AUTH_02) {
  for (int i = 0; i < 3; ++i)
    EXPECT_FALSE(app.login("admin", "wrong"));
  EXPECT_TRUE(app.locked());
  EXPECT_FALSE(app.login("admin", "TestPass123"));
}
TEST_F(Core, TC_AUTH_03) {
  reception();
  app.logout();
  EXPECT_FALSE(app.session());
  EXPECT_THROW(patient(), std::runtime_error);
}
TEST_F(Core, TC_PAT_01) {
  reception();
  auto p = patient();
  EXPECT_EQ(p.id, "P000001");
  EXPECT_EQ(p.name, "Alex Demo");
}
TEST_F(Core, TC_PAT_02) {
  reception();
  auto p = patient();
  auto result = app.searchPatients(p.id);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].contact, "9000000002");
}
TEST_F(Core, TC_PAT_03) {
  reception();
  EXPECT_TRUE(app.searchPatients("P999999").empty());
}
TEST_F(Core, TC_DOC_01) {
  app.login("admin", "TestPass123");
  auto d = app.addDoctor("Dr Lee", "Neurologist", "9000000003");
  EXPECT_EQ(d.id, 2);
}
TEST_F(Core, TC_DOC_02) {
  reception();
  auto doctors = app.doctors("Cardiologist");
  ASSERT_EQ(doctors.size(), 1u);
  EXPECT_EQ(doctors[0].specialization, "Cardiologist");
  EXPECT_TRUE(app.doctors("Dentist").empty());
}
TEST_F(Core, TC_APP_01) {
  reception();
  auto a = app.book(patient().id, 1, "2026-10-19", "09:00");
  EXPECT_EQ(a.status, "Scheduled");
}
TEST_F(Core, TC_APP_02) {
  reception();
  auto p = patient();
  app.book(p.id, 1, "2026-10-19", "09:00");
  EXPECT_THROW(app.book(p.id, 1, "2026-10-19", "09:00"), std::runtime_error);
}
TEST_F(Core, TC_SEC_01) {
  EXPECT_FALSE(app.login("' OR 1=1 --", "anything"));
  EXPECT_FALSE(app.session());
}
TEST_F(Core, RolesEnforcedInServices) {
  app.login("doctor", "TestPass123");
  EXPECT_THROW(patient(), std::runtime_error);
  EXPECT_THROW(app.addDoctor("No", "No", "9000000001"), std::runtime_error);
  EXPECT_THROW(app.provision("bad", "TestPass123", "Admin"),
               std::runtime_error);
  EXPECT_THROW(app.book("P000001", 1, "2026-10-19", "09:00"),
               std::runtime_error);
}
TEST_F(Core, HashesAreSaltedAndNeverPlaintext) {
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
  reception();
  patient();
  EXPECT_EQ(app.searchPatients("Alex").size(), 1u);
  EXPECT_TRUE(app.searchPatients("%").empty());
  auto p = app.registerPatient("O'Neil", 20, "Male", "9000000004", "A+");
  EXPECT_EQ(app.searchPatients("O'Neil")[0].id, p.id);
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
  reception();
  auto p = patient();
  app.updateContact(p.id, "9000000008");
  EXPECT_EQ(app.searchPatients(p.id)[0].contact, "9000000008");
  EXPECT_THROW(app.updateContact("P999999", "9000000008"), std::runtime_error);
  EXPECT_THROW(app.updateContact(p.id, "invalid"), std::invalid_argument);
  app.logout();
  app.login("doctor", "TestPass123");
  EXPECT_THROW(app.updateContact(p.id, "9000000008"), std::runtime_error);
}
TEST_F(Core, TC_APP_03) {
  reception();
  auto p = patient();
  auto a = app.book(p.id, 1, "2026-10-19", "09:00");
  app.cancel(a.id);
  auto q = db.prepare("SELECT status FROM appointments WHERE id=?");
  q.bind(1, a.id);
  ASSERT_TRUE(q.step());
  EXPECT_EQ(q.text(0), "Canceled");
  EXPECT_NO_THROW(app.book(p.id, 1, "2026-10-19", "09:00"));
  EXPECT_THROW(app.cancel(a.id), std::runtime_error);
  EXPECT_THROW(app.cancel(999), std::runtime_error);
}
TEST_F(Core, RescheduleIsAtomic) {
  reception();
  auto p = patient();
  auto a = app.book(p.id, 1, "2026-10-19", "09:00");
  app.book(p.id, 1, "2026-10-19", "09:15");
  EXPECT_THROW(app.reschedule(a.id, "2026-10-19", "09:15"), std::runtime_error);
  {
    auto q = db.prepare("SELECT appt_time FROM appointments WHERE id=?");
    q.bind(1, a.id);
    ASSERT_TRUE(q.step());
    EXPECT_EQ(q.text(0), "09:00");
  }
  EXPECT_THROW(app.reschedule(a.id, "2026-02-30", "09:00"),
               std::invalid_argument);
  EXPECT_NO_THROW(app.reschedule(a.id, "2026-10-19", "09:30"));
  EXPECT_NO_THROW(app.book(p.id, 1, "2026-10-19", "09:00"));
  app.cancel(a.id);
  EXPECT_THROW(app.reschedule(a.id, "2026-10-19", "09:45"), std::runtime_error);
}
TEST_F(Core, DoctorsSeeOnlyOwnSchedule) {
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
  app.login("doctor", "TestPass123");
  auto result = app.mySchedule("2026-10-19");
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].id, a.id);
  EXPECT_THROW(app.complete(other.id), std::runtime_error);
  EXPECT_THROW(app.mySchedule("bad"), std::invalid_argument);
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
  TempDirectory temp;
  app.exportPatients((temp.path / "patients.csv").string());
  app.exportPatients((temp.path / "patients.txt").string());
  app.exportDaily("2026-10-19", (temp.path / "daily.csv").string());
  app.exportDaily("2026-10-19", (temp.path / "daily.txt").string());
  EXPECT_NE(
      readFile(temp.path / "patients.csv").find("\"Demo, \"\"Quoted\"\"\""),
      std::string::npos);
  EXPECT_NE(readFile(temp.path / "patients.txt").find(p.id), std::string::npos);
  EXPECT_NE(readFile(temp.path / "daily.csv").find("2026-10-19,1,1,0,0"),
            std::string::npos);
  EXPECT_NE(
      readFile(temp.path / "daily.txt").find("2026-10-19 | 1 | 1 | 0 | 0"),
      std::string::npos);
}
TEST_F(Core, ReportErrorsAndPermissions) {
  TempDirectory temp;
  auto path = (temp.path / "patients.csv").string();
  reception();
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
