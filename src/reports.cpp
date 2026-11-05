#include "hms/hms.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace hms {
namespace {
/** @brief Quote a CSV field and double embedded quotes per RFC 4180. */
std::string csv(const std::string &value) {
  std::string result = "\"";
  for (char c : value) {
    if (c == '\"')
      result += '\"';
    result += c;
  }
  return result + '\"';
}
/** @brief Validate the extension, create parent folders, and open the output. */
std::ofstream reportFile(const std::string &path) {
  const std::filesystem::path file(path);
  const auto extension = file.extension().string();
  if (extension != ".csv" && extension != ".txt")
    throw std::invalid_argument("Report filename must end in .csv or .txt");
  if (!file.parent_path().empty())
    std::filesystem::create_directories(file.parent_path());
  std::ofstream out(file, std::ios::binary);
  if (!out)
    throw std::runtime_error("Cannot open report file");
  out.exceptions(std::ios::badbit | std::ios::failbit);
  return out;
}
} // namespace
/** @brief Count all appointments and each current status for an Admin.
 * The date is the appointment date, including appointments since canceled.
 */
DailyReport HMS::dailyReport(const std::string &date) {
  require({"Admin"});
  weekday(date);
  auto q = db_.prepare("SELECT "
                       "COUNT(*),COALESCE(SUM(status='Scheduled'),0),COALESCE("
                       "SUM(status='Canceled'),0),COALESCE(SUM(status='"
                       "Completed'),0) FROM appointments WHERE appt_date=?");
  q.bind(1, date);
  q.step();
  return {q.integer(0), q.integer(1), q.integer(2), q.integer(3)};
}
/** @brief Fetch the complete Admin roster with all six patient fields. */
std::vector<Patient> HMS::patientReport() {
  require({"Admin"});
  auto q = db_.prepare("SELECT public_id,name,age,gender,contact,blood_group "
                       "FROM patients ORDER BY id");
  std::vector<Patient> patients;
  while (q.step())
    patients.push_back(
        {q.text(0), q.text(1), q.integer(2), q.text(3), q.text(4), q.text(5)});
  return patients;
}

/** @brief Export the Admin roster with escaped CSV fields and CRLF records. */
void HMS::exportPatients(const std::string &path) {
  const auto patients = patientReport();
  auto out = reportFile(path);
  const bool comma = std::filesystem::path(path).extension() == ".csv";
  out << (comma ? "Patient ID,Name,Age,Gender,Contact,Blood Group\r\n"
                : "Patient ID | Name | Age | Gender | Contact | Blood Group\n");
  for (const auto &patient : patients) {
    const std::array<std::string, 6> fields = {
        patient.id, patient.name, std::to_string(patient.age),
        patient.gender, patient.contact, patient.bloodGroup};
    for (std::size_t col = 0; col < fields.size(); ++col) {
      if (col)
        out << (comma ? "," : " | ");
      out << (comma ? csv(fields[col]) : fields[col]);
    }
    out << (comma ? "\r\n" : "\n");
  }
  out.close();
}

/** @brief Export Admin totals with CRLF CSV records or pipe-delimited text. */
void HMS::exportDaily(const std::string &date, const std::string &path) {
  const auto report = dailyReport(date);
  auto out = reportFile(path);
  const bool comma = std::filesystem::path(path).extension() == ".csv";
  out << (comma ? "Date,Total Booked,Scheduled,Canceled,Completed\r\n"
                : "Date | Total Booked | Scheduled | Canceled | Completed\n");
  const auto separator = comma ? "," : " | ";
  out << date << separator << report.totalBooked << separator
      << report.scheduled << separator << report.canceled << separator
      << report.completed << (comma ? "\r\n" : "\n");
  out.close();
}
} // namespace hms
