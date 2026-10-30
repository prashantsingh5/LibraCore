#pragma once
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <memory>
namespace libracore {
using Json = nlohmann::json;
struct Error : std::runtime_error {
  std::string code;
  Error(std::string c, std::string message)
      : std::runtime_error(std::move(message)), code(std::move(c)) {}
};
// Application operations, never caller-provided SQL. Stateless per-call
// connection/transaction.
class IRepository {
public:
  virtual ~IRepository() = default;
  virtual Json execute(const std::string &operation,
                       const Json &parameters) = 0;
};
class ConnectionPool;
class PostgresRepository final : public IRepository {
public:
  explicit PostgresRepository(std::string connection);
  Json execute(const std::string &, const Json &) override;

private:
  std::string connection_;
  std::shared_ptr<ConnectionPool> pool_;
};
Json money(std::int64_t minor, const std::string &currency);
Json validateResource(const Json &);
} // namespace libracore
