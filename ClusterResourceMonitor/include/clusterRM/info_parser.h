#ifndef MONITER_INFO_PARSER_H_
#define MONITER_INFO_PARSER_H_

#include "../../3rdparty/json/json.hpp"
#include "smq_meta.h"
// BUG:this file can be included in multiple files, which may cause redefinition of functions
namespace moniter {

void to_json(nlohmann::json& j, const moniter::StaticInfo::gpu& g) {
  j = nlohmann::json{{"minor_number", g.minor_number},
                     {"model_name", g.model_name},
                     {"gpu_mem_total", g.gpu_mem_total}};
}

void from_json(const nlohmann::json& j, moniter::StaticInfo::gpu& g) {
  j.at("minor_number").get_to(g.minor_number);
  j.at("model_name").get_to(g.model_name);
  j.at("gpu_mem_total").get_to(g.gpu_mem_total);
}

void to_json(nlohmann::json& j, const moniter::StaticInfo::cpu& c) {
  j = nlohmann::json{{"physical_id", c.physical_id}, {"model_name", c.model_name}};
}

void from_json(const nlohmann::json& j, moniter::StaticInfo::cpu& c) {
  j.at("physical_id").get_to(c.physical_id);
  j.at("model_name").get_to(c.model_name);
}

void to_json(nlohmann::json& j, const moniter::DynamicInfo::gpu_usage& g) {
  j = nlohmann::json{
      {"minor_number", g.minor_number}, {"gpu_util", g.gpu_util}, {"mem_util", g.mem_util}};
}

void from_json(const nlohmann::json& j, moniter::DynamicInfo::gpu_usage& g) {
  j.at("minor_number").get_to(g.minor_number);
  j.at("gpu_util").get_to(g.gpu_util);
  j.at("mem_util").get_to(g.mem_util);
}

void to_json(nlohmann::json& j, const moniter::DynamicInfoMeta& d) {
  j = nlohmann::json{
      {"available_ram", d.available_ram}, {"cpu_usage", d.cpu_usage}, {"gpu_usage", d.gpu_usage}};
}

void from_json(const nlohmann::json& j, moniter::DynamicInfoMeta& d) {
  j.at("available_ram").get_to(d.available_ram);
  j.at("cpu_usage").get_to(d.cpu_usage);
  j.at("gpu_usage").get_to(d.gpu_usage);
}

void to_json(nlohmann::json& j, const moniter::StaticInfoMeta& s) {
  j = nlohmann::json{{"cpu", s.cpu_models}, {"gpu", s.gpu_models}, {"total_ram", s.total_ram}};
}

void from_json(const nlohmann::json& j, moniter::StaticInfoMeta& s) {
  j.at("cpu").get_to(s.cpu_models);
  j.at("gpu").get_to(s.gpu_models);
  j.at("total_ram").get_to(s.total_ram);
}

void to_json(nlohmann::json& j, const moniter::NetworkInfoMeta& n) {
  j = n.bandwidth;
}

void from_json(const nlohmann::json& j, moniter::NetworkInfoMeta& n) {
  j.get_to(n.bandwidth);
}

void to_json(nlohmann::json& j, const moniter::SmqMeta& s) {
  j["id"] = s.id;
  // TODO improve the logic
  if (s.dynamic_info.available_ram > 10) {
    j["dynamic_info"] = s.dynamic_info;
  }

  if (!s.static_info.cpu_models.empty()) {
    j["static_info"] = s.static_info;
  }

  if (!s.network_info.bandwidth.empty()) {
    j["network_info"] = s.network_info;
  }
}

void from_json(const nlohmann::json& j, moniter::SmqMeta& s) {
  j.at("id").get_to(s.id);
  if (j.find("dynamic_info") == j.end() || j.at("dynamic_info").is_null()) {
    s.dynamic_info = {};
  } else {
    j.at("dynamic_info").get_to(s.dynamic_info);
  }
  if (j.find("static_info") == j.end() || j.at("static_info").is_null()) {
    s.static_info = {};
  } else {
    j.at("static_info").get_to(s.static_info);
  }
  if (j.find("network_info") == j.end() || j.at("network_info").is_null()) {
    s.network_info = {};
  } else {
    j.at("network_info").get_to(s.network_info);
  }
}

void to_json(nlohmann::json& j, const moniter::SignalMeta& k) {
  j = nlohmann::json{{"ksignal", k.ksignal}, {"id", k.id}};

  if (k.ksignal == 0) {
    j["iperf_port"] = k.iperf_port;
  }

  if (k.ksignal != 0) {
    j["smq_meta"] = k.smq_meta;
  }
  if (!k.ip.empty()) {
    j["ip"] = k.ip;
  }

  if (!k.test_targets.empty()) {
    j["test_targets"] = k.test_targets;
  }
}

void from_json(const nlohmann::json& j, moniter::SignalMeta& k) {
  j.at("ksignal").get_to(k.ksignal);
  j.at("id").get_to(k.id);
  if (j.find("smq_meta") == j.end() || j.at("smq_meta").is_null()) {
    k.smq_meta = {};
  } else {
    j.at("smq_meta").get_to(k.smq_meta);
  }
  if (j.find("iperf_port") == j.end() || j.at("iperf_port").is_null()) {
    k.iperf_port = {};
  } else {
    j.at("iperf_port").get_to(k.iperf_port);
  }
  if (j.find("test_targets") == j.end() || j.at("test_targets").is_null()) {
    k.test_targets = {};
  } else {
    j.at("test_targets").get_to(k.test_targets);
  }
  if (j.find("ip") == j.end() || j.at("ip").is_null()) {
    k.ip = {};
  } else {
    j.at("ip").get_to(k.ip);
  }
}

SmqMeta convert_to_meta(const std::string& info) {
  nlohmann::json j = nlohmann::json::parse(info);
  SmqMeta smq_meta = j.get<SmqMeta>();
  return smq_meta;
}

SignalMeta parse_signal(const std::string& info) {
  nlohmann::json j = nlohmann::json::parse(info);
  SignalMeta signal_meta = j.get<SignalMeta>();
  return signal_meta;
}
}  // namespace moniter
#endif