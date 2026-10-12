#include "hms/hms.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace hms {
Patient HMS::registerPatient(const std::string &name, int age,
                             const std::string &gender,
                             const std::string &contact,
                             const std::string &blood) {
  require({"Receptionist"});
  validateText(name, 100, "name");
  validateContact(contact);
  if (age < 0 || age > 130)
    throw std::invalid_argument("Age must be 0 to 130");
  const std::vector<std::string> genders = {"Male", "Female", "Other"},
                                 groups = {"A+",  "A-",  "B+", "B-",
                                           "AB+", "AB-", "O+", "O-"};
  if (std::find(genders.begin(), genders.end(), gender) == genders.end())
    throw std::invalid_argument("Gender must be Male, Female or Other");
  if (std::find(groups.begin(), groups.end(), blood) == groups.end())
    throw std::invalid_argument("Invalid blood group");
  Transaction tx(db_);
  auto q =
      db_.prepare("INSERT INTO patients(name,age,gender,contact,blood_group) "
                  "VALUES(?,?,?,?,?)");
  q.bind(1, name);
  q.bind(2, age);
  q.bind(3, gender);
  q.bind(4, contact);
  q.bind(5, blood);
  q.step();
  const int id = db_.lastId();
  std::ostringstream formatted;
  formatted << 'P' << std::setw(6) << std::setfill('0') << id;
  auto update = db_.prepare("UPDATE patients SET public_id=? WHERE id=?");
  update.bind(1, formatted.str());
  update.bind(2, id);
  update.step();
  tx.commit();
  return {formatted.str(), name, age, gender, contact, blood};
}
std::vector<Patient> HMS::searchPatients(const std::string &term) {
  require({"Receptionist", "Doctor"});
  validateText(term, 100, "search term");
  std::string escaped;
  for (char c : term) {
    if (c == '%' || c == '_' || c == '!')
      escaped += '!';
    escaped += c;
  }
  auto q = db_.prepare(
      "SELECT public_id,name,age,gender,contact,blood_group FROM patients "
      "WHERE public_id=? OR name LIKE ? ESCAPE '!' ORDER BY id");
  q.bind(1, term);
  q.bind(2, "%" + escaped + "%");
  std::vector<Patient> result;
  while (q.step())
    result.push_back(
        {q.text(0), q.text(1), q.integer(2), q.text(3), q.text(4), q.text(5)});
  return result;
}
} // namespace hms
