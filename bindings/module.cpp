#include "libracore/Repository.h"
#include <pybind11/pybind11.h>
namespace py = pybind11;
PYBIND11_MODULE(_libracore, m) {
  m.doc() =
      "LibraCore V2 application boundary; Python-native structured values.";
  py::class_<libracore::PostgresRepository>(m, "Application")
      .def(py::init<std::string>())
      .def("execute", [](libracore::PostgresRepository &app,
                         const std::string &op, py::dict args) {
        auto json = py::module_::import("json");
        auto input = libracore::Json::parse(
            json.attr("dumps")(args).cast<std::string>());
        libracore::Json result;
        {
          py::gil_scoped_release release;
          try {
            result = {{"ok", true}, {"data", app.execute(op, input)}};
          } catch (const libracore::Error &e) {
            result = {{"ok", false},
                      {"error",
                       {{"code", e.code},
                        {"message", e.what()},
                        {"details", libracore::Json::object()}}}};
          }
        }
        return json.attr("loads")(result.dump());
      });
  m.def("validate_resource", [](py::dict args) {
    auto json = py::module_::import("json");
    return json.attr("loads")(
        libracore::validateResource(
            libracore::Json::parse(
                json.attr("dumps")(args).cast<std::string>()))
            .dump());
  });
  m.def("money", [](std::int64_t minor, const std::string &currency) {
    return py::module_::import("json").attr("loads")(
        libracore::money(minor, currency).dump());
  });
}
