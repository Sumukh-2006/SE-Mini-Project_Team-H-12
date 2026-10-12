#include "hms/hms.hpp"
#include <stdexcept>

namespace hms {
bool HMS::available(int id, const std::string &date, const std::string &time,
                    int exclude) {
  const int day = weekday(date), at = minutes(time);
  if (at % 15 != 0)
    return false;
  auto q = db_.prepare("SELECT 1 FROM availability WHERE doctor_id=? AND "
                       "weekday=? AND start_min<=? AND end_min>=?");
  q.bind(1, id);
  q.bind(2, day);
  q.bind(3, at);
  q.bind(4, at + 15);
  if (!q.step())
    return false;
  auto busy = db_.prepare(
      "SELECT 1 FROM appointments WHERE doctor_id=? AND appt_date=? AND "
      "appt_time=? AND status!='Canceled' AND id!=?");
  busy.bind(1, id);
  busy.bind(2, date);
  busy.bind(3, time);
  busy.bind(4, exclude);
  return !busy.step();
}
Appointment HMS::book(const std::string &patient, int doctor,
                      const std::string &date, const std::string &time) {
  require({"Receptionist"});
  Transaction tx(db_);
  auto p = db_.prepare("SELECT id FROM patients WHERE public_id=?");
  p.bind(1, patient);
  if (!p.step())
    throw std::runtime_error("Patient not found");
  if (!available(doctor, date, time))
    throw std::runtime_error("Doctor unavailable or slot already booked");
  auto q = db_.prepare("INSERT INTO "
                       "appointments(patient_id,doctor_id,appt_date,appt_time,"
                       "status) VALUES(?,?,?,?,'Scheduled')");
  q.bind(1, p.integer(0));
  q.bind(2, doctor);
  q.bind(3, date);
  q.bind(4, time);
  q.step();
  int id = db_.lastId();
  tx.commit();
  return {id, patient, doctor, date, time, "Scheduled"};
}
} // namespace hms
