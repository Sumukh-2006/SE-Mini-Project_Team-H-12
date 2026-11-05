#include "hms/hms.hpp"
#include <stdexcept>

namespace hms {
/**
 * @brief Checks doctor availability and slot conflicts (FR-APP-02).
 */
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

/**
 * @brief Books a 15-minute appointment slot inside a database transaction (FR-APP-01, FR-APP-02).
 */
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
/**
 * @brief Cancels an existing scheduled appointment and frees the consultation slot (FR-APP-03).
 *
 * Enforces Receptionist-only role authorization. Atomically transitions the appointment
 * record from 'Scheduled' to 'Canceled' within a database transaction, immediately freeing
 * the time slot for subsequent bookings while preserving the historical record in the database.
 *
 * @param id Unique database identifier of the appointment to cancel.
 * @throws std::runtime_error If caller is not Receptionist, appointment does not exist,
 *                            or appointment status is not 'Scheduled'.
 */
void HMS::cancel(int id) {
  require({"Receptionist"});
  Transaction tx(db_);
  auto existing = db_.prepare("SELECT status FROM appointments WHERE id=?");
  existing.bind(1, id);
  if (!existing.step())
    throw std::runtime_error("Appointment not found");
  if (existing.text(0) != "Scheduled")
    throw std::runtime_error("Only Scheduled appointments can be canceled");
  auto q = db_.prepare("UPDATE appointments SET status='Canceled' WHERE id=?");
  q.bind(1, id);
  q.step();
  tx.commit();
}

/**
 * @brief Atomically reschedules an appointment to a new date and time slot (FR-APP-04).
 *
 * Enforces Receptionist-only role authorization. Validates that the appointment exists in
 * 'Scheduled' status, checks doctor weekly availability and slot boundaries for the target date
 * and time (excluding the current appointment ID), and updates the record within a database
 * transaction. If validation or availability fails, the transaction is rolled back, preserving
 * the original appointment date and time.
 *
 * @param id Unique database identifier of the appointment to reschedule.
 * @param date Target date formatted as YYYY-MM-DD.
 * @param time Target time formatted as HH:MM on a 15-minute boundary.
 * @throws std::runtime_error If caller is not Receptionist, appointment does not exist,
 *                            appointment is not 'Scheduled', or the doctor/slot is unavailable.
 * @throws std::invalid_argument If date or time formatting fails validation.
 */
void HMS::reschedule(int id, const std::string &date, const std::string &time) {
  require({"Receptionist"});
  Transaction tx(db_);
  auto existing =
      db_.prepare("SELECT doctor_id,status FROM appointments WHERE id=?");
  existing.bind(1, id);
  if (!existing.step())
    throw std::runtime_error("Appointment not found");
  if (existing.text(1) != "Scheduled")
    throw std::runtime_error("Only Scheduled appointments can be rescheduled");
  if (!available(existing.integer(0), date, time, id))
    throw std::runtime_error("Doctor unavailable or slot already booked");
  auto q =
      db_.prepare("UPDATE appointments SET appt_date=?,appt_time=? WHERE id=?");
  q.bind(1, date);
  q.bind(2, time);
  q.bind(3, id);
  q.step();
  tx.commit();
}

/**
 * @brief Retrieves scheduled appointments for the authenticated doctor on a given date (FR-APP-05).
 *
 * Enforces Doctor-only role authorization. Validates the date format and queries all appointments
 * in 'Scheduled' status assigned strictly to the authenticated doctor's registered ID,
 * returning them in chronological order by appointment time.
 *
 * @param date Date formatted as YYYY-MM-DD.
 * @return std::vector<Appointment> List of scheduled appointments for the doctor.
 * @throws std::runtime_error If caller is not authenticated as a Doctor.
 * @throws std::invalid_argument If date formatting fails validation.
 */
std::vector<Appointment> HMS::mySchedule(const std::string &date) {
  require({"Doctor"});
  weekday(date);
  auto q = db_.prepare(
      "SELECT a.id,p.public_id,a.doctor_id,a.appt_date,a.appt_time,a.status "
      "FROM appointments a JOIN patients p ON a.patient_id=p.id WHERE "
      "a.doctor_id=? AND a.appt_date=? AND a.status='Scheduled' ORDER BY "
      "a.appt_time");
  q.bind(1, session_->doctorId);
  q.bind(2, date);
  std::vector<Appointment> result;
  while (q.step())
    result.push_back({q.integer(0), q.text(1), q.integer(2), q.text(3),
                      q.text(4), q.text(5)});
  return result;
}
void HMS::complete(int id) {
  require({"Doctor"});
  Transaction tx(db_);
  auto existing =
      db_.prepare("SELECT doctor_id,status FROM appointments WHERE id=?");
  existing.bind(1, id);
  if (!existing.step())
    throw std::runtime_error("Appointment not found");
  if (existing.integer(0) != session_->doctorId)
    throw std::runtime_error("Permission denied");
  if (existing.text(1) != "Scheduled")
    throw std::runtime_error("Only Scheduled appointments can be completed");
  auto q = db_.prepare("UPDATE appointments SET status='Completed' WHERE id=?");
  q.bind(1, id);
  q.step();
  tx.commit();
}
} // namespace hms
