#include "hms/database.hpp"
#include <chrono>
#include <iostream>
#include <thread>
/** Fault-injection helper deliberately leaves a production DAL transaction
 * uncommitted. */
int main(int argc, char **argv) {
  if (argc != 2)
    return 1;
  hms::Database db(argv[1]);
  hms::Transaction tx(db);
  auto q = db.prepare("UPDATE patients SET contact=? WHERE public_id=?");
  q.bind(1, "9999999999");
  q.bind(2, "P000001");
  q.step();
  std::cout << "UNCOMMITTED" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(30));
  return 1;
}
