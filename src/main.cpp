#include "hms/hms.hpp"
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#endif

namespace {
std::string input(const std::string &prompt, bool secret = false) {
  std::cout << prompt << std::flush;
#ifndef _WIN32
  struct EchoGuard {
    termios old{};
    bool changed = false;
    explicit EchoGuard(bool hide) {
      if (hide && isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &old) == 0) {
        auto current = old;
        current.c_lflag &= ~ECHO;
        changed = tcsetattr(STDIN_FILENO, TCSANOW, &current) == 0;
      }
    }
    ~EchoGuard() {
      if (changed)
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
    }
  } guard(secret);
#else
  (void)secret;
#endif
  std::string value;
  if (!std::getline(std::cin, value))
    throw std::runtime_error("Input closed");
  if (secret)
    std::cout << '\n';
  if (value.size() > 4096)
    throw std::invalid_argument("Input too long");
  return value;
}
int number(const std::string &prompt) {
  const auto text = input(prompt);
  int value = 0;
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size())
    throw std::invalid_argument("Enter a whole number");
  return value;
}
void showPatients(const std::vector<hms::Patient> &patients) {
  if (patients.empty())
    std::cout << "Patient not found\n";
  for (const auto &p : patients)
    std::cout << p.id << " | " << p.name << " | age " << p.age << " | "
              << p.gender << " | " << p.contact << " | " << p.bloodGroup
              << '\n';
}
void demo(hms::Database &db, hms::HMS &app) {
  const char *supplied = std::getenv("HMS_DEMO_PASSWORD");
  if (!supplied)
    throw std::runtime_error(
        "Set HMS_DEMO_PASSWORD to an 8+ character demonstration password");
  auto count = db.prepare("SELECT COUNT(*) FROM users");
  count.step();
  if (count.integer(0) != 0)
    throw std::runtime_error(
        "Demo seeding requires an empty database; choose a new --db path");
  app.bootstrap("admin", supplied);
  app.login("admin", supplied);
  auto doctor = app.addDoctor("Dr Mira Rao", "Cardiologist", "9000000001");
  for (int day = 0; day < 7; ++day)
    app.setAvailability(doctor.id, day, "09:00", "17:00");
  app.provision("reception", supplied, "Receptionist");
  app.provision("doctor", supplied, "Doctor", doctor.id);
  app.logout();
  app.login("reception", supplied);
  app.registerPatient("Alex Demo", 29, "Other", "9000000002", "O+");
  app.logout();
  std::cout
      << "Demo database initialized with fictional data.\nAccounts: admin, "
         "reception, doctor\nUse your supplied demonstration password.\n";
}
void menu(hms::HMS &app) {
  const auto role = app.session()->role;
  std::cout << "\n" << role << " menu\n";
  if (role == "Admin")
    std::cout << "1 Add doctor\n2 Set weekly availability\n3 List doctors\n4 "
                 "Create user account\n";
  if (role == "Receptionist")
    std::cout << "1 Register patient\n2 Search patients\n3 Filter doctors\n4 "
                 "Book appointment\n";
  if (role == "Doctor")
    std::cout << "1 Search patients\n";
  std::cout << "0 Logout\n";
  const int choice = number("Choice: ");
  if (choice == 0) {
    app.logout();
    std::cout << "Logged out\n";
    return;
  }
  if (role == "Admin") {
    if (choice == 1) {
      auto name = input("Doctor name: ");
      auto spec = input("Specialization: ");
      auto contact = input("Contact: ");
      auto d = app.addDoctor(name, spec, contact);
      std::cout << "Doctor added. Doctor ID: " << d.id << '\n';
    } else if (choice == 2) {
      int id = number("Doctor ID: "),
          day = number("Weekday (0 Sunday, 1 Monday ... 6 Saturday): ");
      auto start = input("Start (HH:MM): ");
      auto end = input("End (HH:MM): ");
      app.setAvailability(id, day, start, end);
      std::cout << "Weekly availability saved\n";
    } else if (choice == 3) {
      for (const auto &d : app.doctors(""))
        std::cout << d.id << " | " << d.name << " | " << d.specialization
                  << '\n';
    } else if (choice == 4) {
      auto name = input("New username: ");
      auto password = input("New password: ", true);
      auto r = input("Role (Admin/Receptionist/Doctor): ");
      int id = r == "Doctor" ? number("Linked Doctor ID: ") : 0;
      app.provision(name, password, r, id);
      std::cout << "User created\n";
    } else
      throw std::invalid_argument("Unknown menu option");
  } else if (role == "Receptionist") {
    if (choice == 1) {
      auto name = input("Patient name: ");
      int age = number("Age: ");
      auto gender = input("Gender (Male/Female/Other): ");
      auto contact = input("Contact: ");
      auto blood = input("Blood group: ");
      auto p = app.registerPatient(name, age, gender, contact, blood);
      std::cout << "Patient registered. Patient ID: " << p.id << '\n';
    } else if (choice == 2)
      showPatients(app.searchPatients(input("Patient ID or name: ")));
    else if (choice == 3) {
      auto spec = input("Specialization (blank for all): ");
      for (const auto &d : app.doctors(spec))
        std::cout << d.id << " | " << d.name << " | " << d.specialization
                  << '\n';
    } else if (choice == 4) {
      auto patient = input("Patient ID: ");
      int doctor = number("Doctor ID: ");
      auto date = input("Date (YYYY-MM-DD): ");
      auto time = input("Time (HH:MM): ");
      auto a = app.book(patient, doctor, date, time);
      std::cout << "Appointment booked. Appointment ID: " << a.id << " | "
                << a.date << " " << a.time << " | " << a.status << '\n';
    } else
      throw std::invalid_argument("Unknown menu option");
  } else if (choice == 1)
    showPatients(app.searchPatients(input("Patient ID or name: ")));
  else
    throw std::invalid_argument("Unknown menu option");
}
} // namespace
int main(int argc, char **argv) {
  try {
    std::string path = ".local/hms.db";
    bool seed = false;
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--db" && i + 1 < argc)
        path = argv[++i];
      else if (arg == "--demo-seed")
        seed = true;
      else if (arg == "--help") {
        std::cout << "Usage: hms_app [--db PATH] [--demo-seed]\nFirst run "
                     "creates an administrator interactively.\n";
        return 0;
      } else
        throw std::invalid_argument("Unknown or incomplete argument: " + arg);
    }
    hms::Database db(path);
    hms::HMS app(db);
    if (seed) {
      demo(db, app);
      return 0;
    }
    std::cout << "Hospital Management System | Sprint 1\n15-minute appointment "
                 "slots | Local SQLite storage\n";
    auto q = db.prepare("SELECT COUNT(*) FROM users");
    q.step();
    if (q.integer(0) == 0) {
      std::cout << "First-run administrator setup\n";
      auto name = input("Admin username: ");
      auto password = input("Admin password: ", true);
      app.bootstrap(name, password);
      std::cout << "Administrator initialized\n";
    }
    while (!app.locked()) {
      try {
        if (!app.session()) {
          auto name = input("Username (q to quit): ");
          if (name == "q")
            break;
          auto password = input("Password: ", true);
          if (app.login(name, password))
            std::cout << "Welcome, " << name << '\n';
          else
            std::cout << "Invalid credentials"
                      << (app.locked() ? ". Three attempts used; access denied."
                                       : ". Try again.")
                      << '\n';
        } else
          menu(app);
      } catch (const std::exception &e) {
        if (!std::cin)
          break;
        std::cout << "Error: " << e.what() << '\n';
      }
    }
    return app.locked() ? 2 : 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
}
