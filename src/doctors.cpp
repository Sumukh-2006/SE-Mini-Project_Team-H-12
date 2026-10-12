#include "hms/hms.hpp"
#include <cctype>
#include <stdexcept>

namespace hms {
int minutes(const std::string &time) {
  if (time.size() != 5 || time[2] != ':' ||
      !std::isdigit(static_cast<unsigned char>(time[0])) ||
      !std::isdigit(static_cast<unsigned char>(time[1])) ||
      !std::isdigit(static_cast<unsigned char>(time[3])) ||
      !std::isdigit(static_cast<unsigned char>(time[4])))
    throw std::invalid_argument("Time must be HH:MM");
  int h = std::stoi(time.substr(0, 2)), m = std::stoi(time.substr(3, 2));
  if (h > 23 || m > 59)
    throw std::invalid_argument("Invalid time");
  return h * 60 + m;
}
int weekday(const std::string &date) {
  if (date.size() != 10 || date[4] != '-' || date[7] != '-')
    throw std::invalid_argument("Date must be YYYY-MM-DD");
  for (std::size_t i = 0; i < date.size(); ++i)
    if (i != 4 && i != 7 && !std::isdigit(static_cast<unsigned char>(date[i])))
      throw std::invalid_argument("Invalid date");
  int y = std::stoi(date.substr(0, 4)), m = std::stoi(date.substr(5, 2)),
      d = std::stoi(date.substr(8, 2));
  const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (y < 1900 || m < 1 || m > 12 || d < 1 ||
      d > days[m - 1] +
              (m == 2 && y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)))
    throw std::invalid_argument("Invalid calendar date");
  const int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3)
    --y;
  return (y + y / 4 - y / 100 + y / 400 + offsets[m - 1] + d) % 7;
}
Doctor HMS::addDoctor(const std::string &name, const std::string &spec,
                      const std::string &contact) {
  require({"Admin"});
  validateText(name, 100, "name");
  validateText(spec, 50, "specialization");
  validateContact(contact);
  auto q = db_.prepare(
      "INSERT INTO doctors(name,specialization,contact) VALUES(?,?,?)");
  q.bind(1, name);
  q.bind(2, spec);
  q.bind(3, contact);
  q.step();
  return {db_.lastId(), name, spec, contact};
}
void HMS::setAvailability(int id, int day, const std::string &start,
                          const std::string &end) {
  require({"Admin"});
  int a = minutes(start), b = minutes(end);
  if (day < 0 || day > 6 || a >= b || a % 15 != 0 || b % 15 != 0)
    throw std::invalid_argument(
        "Use weekday 0-6 and increasing 15-minute boundaries");
  auto q = db_.prepare(
      "INSERT INTO availability VALUES(?,?,?,?) ON CONFLICT(doctor_id,weekday) "
      "DO UPDATE SET start_min=excluded.start_min,end_min=excluded.end_min");
  q.bind(1, id);
  q.bind(2, day);
  q.bind(3, a);
  q.bind(4, b);
  q.step();
}
std::vector<Doctor> HMS::doctors(const std::string &spec) {
  require({"Admin", "Receptionist"});
  auto q =
      db_.prepare("SELECT id,name,specialization,contact FROM doctors WHERE "
                  "?='' OR specialization=? COLLATE NOCASE ORDER BY id");
  q.bind(1, spec);
  q.bind(2, spec);
  std::vector<Doctor> result;
  while (q.step())
    result.push_back({q.integer(0), q.text(1), q.text(2), q.text(3)});
  return result;
}
} // namespace hms
