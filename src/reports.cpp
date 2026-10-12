#include "hms/hms.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace hms {
namespace {
std::string csv(const std::string &value) {
  std::string result = "\"";
  for (char c : value) {
    if (c == '\"')
      result += '\"';
    result += c;
  }
  return result + '\"';
}
std::ofstream reportFile(const std::string &path) {
  const std::filesystem::path file(path);
  const auto extension = file.extension().string();
  if (extension != ".csv" && extension != ".txt")
    throw std::invalid_argument("Report filename must end in .csv or .txt");
  if (!file.parent_path().empty())
    std::filesystem::create_directories(file.parent_path());
  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("Cannot open report file");
  out.exceptions(std::ios::badbit | std::ios::failbit);
  return out;
}
} // namespace
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
void HMS::exportPatients(const std::string &path) {
  require({"Admin"});
  auto out = reportFile(path);
  const bool comma = std::filesystem::path(path).extension() == ".csv";
  out << (comma ? "Patient ID,Name,Age,Gender,Contact,Blood Group\n"
                : "Patient ID | Name | Age | Gender | Contact | Blood Group\n");
  auto q = db_.prepare("SELECT public_id,name,age,gender,contact,blood_group "
                       "FROM patients ORDER BY id");
  while (q.step()) {
    for (int col = 0; col < 6; ++col) {
      if (col)
        out << (comma ? "," : " | ");
      out << (comma ? csv(q.text(col)) : q.text(col));
    }
    out << '\n';
  }
  out.close();
}
void HMS::exportDaily(const std::string &date, const std::string &path) {
  const auto report = dailyReport(date);
  auto out = reportFile(path);
  const bool comma = std::filesystem::path(path).extension() == ".csv";
  out << (comma ? "Date,Total Booked,Scheduled,Canceled,Completed\n"
                : "Date | Total Booked | Scheduled | Canceled | Completed\n");
  const auto separator = comma ? "," : " | ";
  out << date << separator << report.totalBooked << separator
      << report.scheduled << separator << report.canceled << separator
      << report.completed << '\n';
  out.close();
}
} // namespace hms
