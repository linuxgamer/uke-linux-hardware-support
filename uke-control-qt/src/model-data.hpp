// SPDX-License-Identifier: MIT
#pragma once
#include <QtCore>
#if defined(UKE_CONTROL_SOURCE_HEADERS)
#include <client.hpp>
#else
#include <uke-control/client.hpp>
#endif
namespace uq {
using namespace uke::client;
struct Fixture {
  bool enabled = false;
  QJsonObject results;
  static Fixture load(const QString &path);
  Result result(const std::string &id) const;
  int delay(const std::string &id) const;
};
QString safe(const std::string &text);
QString friendlyTitle(const Action &action);
QString summary(const Action &action);
} // namespace uq
