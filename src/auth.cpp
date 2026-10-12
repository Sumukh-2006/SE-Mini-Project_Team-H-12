#include "hms/hms.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <stdexcept>

namespace hms {
void validateText(const std::string &text, int maxLength,
                  const std::string &field) {
  if (text.empty() || text.size() > static_cast<std::size_t>(maxLength) ||
      std::all_of(text.begin(), text.end(),
                  [](unsigned char c) { return std::isspace(c); }) ||
      std::any_of(text.begin(), text.end(),
                  [](unsigned char c) { return c < 32 || c == 127; }))
    throw std::invalid_argument("Invalid " + field);
}
void validateContact(const std::string &text) {
  validateText(text, 15, "contact");
  const auto count =
      std::count_if(text.begin(), text.end(),
                    [](unsigned char c) { return std::isdigit(c); });
  if (count < 7 || std::any_of(text.begin(), text.end(), [](unsigned char c) {
        return !std::isdigit(c) && c != '+' && c != '-' && c != ' ';
      }))
    throw std::invalid_argument(
        "Contact must contain 7 to 15 digits with optional +, - or spaces");
}
static std::string hex(const unsigned char *bytes, std::size_t size) {
  std::ostringstream out;
  for (std::size_t i = 0; i < size; ++i)
    out << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(bytes[i]);
  return out.str();
}
std::string randomSalt() {
  unsigned char salt[16];
  if (RAND_bytes(salt, sizeof(salt)) != 1)
    throw std::runtime_error("Secure randomness unavailable");
  return hex(salt, sizeof(salt));
}
std::string passwordHash(const std::string &salt, const std::string &password) {
  const std::string input = salt + ":" + password;
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int length = 0;
  if (EVP_Digest(input.data(), input.size(), digest, &length, EVP_sha256(),
                 nullptr) != 1)
    throw std::runtime_error("Password hashing failed");
  return hex(digest, length);
}
void HMS::require(const std::vector<std::string> &roles) const {
  if (!session_ ||
      std::find(roles.begin(), roles.end(), session_->role) == roles.end())
    throw std::runtime_error("Permission denied");
}
static void validateCredentials(const std::string &name,
                                const std::string &password) {
  validateText(name, 50, "username");
  validateText(password, 128, "password");
  if (password.size() < 8)
    throw std::invalid_argument("Password must contain at least 8 characters");
}
void HMS::bootstrap(const std::string &name, const std::string &password) {
  validateCredentials(name, password);
  Transaction tx(db_);
  auto count = db_.prepare("SELECT COUNT(*) FROM users");
  count.step();
  if (count.integer(0) != 0)
    throw std::runtime_error("Administrator already initialized");
  const auto salt = randomSalt();
  auto insert =
      db_.prepare("INSERT INTO users(username,password_hash,salt,role) "
                  "VALUES(?,?,?,'Admin')");
  insert.bind(1, name);
  insert.bind(2, passwordHash(salt, password));
  insert.bind(3, salt);
  insert.step();
  tx.commit();
}
void HMS::provision(const std::string &name, const std::string &password,
                    const std::string &role, int doctorId) {
  require({"Admin"});
  validateCredentials(name, password);
  if (role != "Admin" && role != "Receptionist" && role != "Doctor")
    throw std::invalid_argument("Invalid role");
  if ((role == "Doctor" && doctorId <= 0) ||
      (role != "Doctor" && doctorId != 0))
    throw std::invalid_argument("Invalid doctor link");
  const auto salt = randomSalt();
  auto q = db_.prepare(
      "INSERT INTO users(username,password_hash,salt,role,doctor_id) "
      "VALUES(?,?,?,?,NULLIF(?,0))");
  q.bind(1, name);
  q.bind(2, passwordHash(salt, password));
  q.bind(3, salt);
  q.bind(4, role);
  q.bind(5, doctorId);
  q.step();
}
bool HMS::login(const std::string &name, const std::string &password) {
  if (session_)
    throw std::runtime_error("Logout before logging in again");
  if (locked())
    return false;
  auto q = db_.prepare(
      "SELECT id,username,password_hash,salt,role,COALESCE(doctor_id,0) FROM "
      "users WHERE username=?");
  q.bind(1, name);
  if (q.step()) {
    const auto actual = passwordHash(q.text(3), password), expected = q.text(2);
    if (actual.size() == expected.size() &&
        CRYPTO_memcmp(actual.data(), expected.data(), actual.size()) == 0) {
      session_ = Session{q.integer(0), q.text(1), q.text(4), q.integer(5)};
      attempts_ = 0;
      return true;
    }
  }
  ++attempts_;
  return false;
}
void HMS::logout() { session_.reset(); }
} // namespace hms
