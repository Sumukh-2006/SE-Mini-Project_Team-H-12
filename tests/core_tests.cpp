#include "hms/hms.hpp"
#include <gtest/gtest.h>

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
