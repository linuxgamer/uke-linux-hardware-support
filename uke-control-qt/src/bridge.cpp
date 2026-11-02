// SPDX-License-Identifier: MIT
#include "bridge.hpp"
#if defined(UKE_CONTROL_SOURCE_HEADERS)
#include <frontend.hpp>
#else
#include <uke-control/frontend.hpp>
#endif
#include <climits>
namespace uq {
static QString q(const std::string &s) { return QString::fromStdString(s); }
static QVariantMap item(const std::string &id) {
  const auto &a = action(id);
  return {
      {"id", q(id)},
      {"title", friendlyTitle(a)},
      {"description", summary(a)},
      {"icon", a.effect == Effect::read ? "view-list-details" : "configure"}};
}
QVariantList Bridge::areas() const {
  QVariantList result;
  const QStringList names{"Status",      "Power",           "Display & lights",
                          "Connections", "Audio & sensors", "Updates",
                          "Advanced"};
  const QStringList icons{"dialog-information", "battery",
                          "video-display",      "network-wired",
                          "audio-card",         "system-software-update",
                          "preferences-system"};
  for (int i = 0; i < names.size(); ++i)
    result << QVariantMap{
        {"title", names[i]}, {"icon", icons[i]}, {"index", i}};
  return result;
}
QVariantList Bridge::groups() const {
  QVariantList result;
  auto add = [&](const QString &title,
                 std::initializer_list<const char *> ids) {
    QVariantList entries;
    for (const auto *id : ids)
      entries << item(id);
    result << QVariantMap{{"title", title}, {"entries", entries}};
  };
  switch (section_) {
  case 0:
    add("Device at a glance",
        {"status", "overview.battery", "overview.workload"});
    add("System information",
        {"hardware.status", "overview.desktop", "overview.storage"});
    break;
  case 1:
    add("Battery & charging", {"battery.status", "battery.health",
                               "charge.status", "charge.admission"});
    add("Charging preferences", {"charge.mode", "charge.limit"});
    add("Sleep & wake", {"overview.sleep", "wake.status", "wake.set"});
    break;
  case 2:
    add("Desktop",
        {"desktop.plasma", "desktop.rotation", "desktop.brightness"});
    add("Lights", {"flashlight.status", "flashlight.state", "flashlight.level",
                   "led.status", "led.color", "led.auto"});
    add("Display link", {"display.bits"});
    break;
  case 3:
    add("USB & accessories", {"usb.status"});
    add("Network & NFC", {"ethernet.status", "overview.nfc", "nfc.state"});
    break;
  case 4:
    add("Audio & microphone", {"overview.audio", "microphone.status",
                               "microphone.select", "overview.camera"});
    add("Sensors", {"sensors.list", "sensors.once", "sensors.watch"});
    break;
  case 5:
    add("Channel & update review", {"channel.status", "update.preview"});
    add("Apply reviewed work", {"update.apply"});
    break;
  }
  return result;
}
QVariantList Bridge::search(const QString &text) const {
  QVariantList entries;
  for (const auto &a : actions())
    if ((q(a.id) + " " + friendlyTitle(a) + " " + q(a.description.en))
            .contains(text, Qt::CaseInsensitive))
      entries << item(a.id);
  return entries;
}
Bridge::Bridge(Fixture fixture, bool readOnly, QObject *parent)
    : QObject(parent), fixture_(std::move(fixture)), readOnly_(readOnly) {
  poll_.setInterval(40);
  connect(&poll_, &QTimer::timeout, this, [this] {
    if (!query_)
      return;
    query_->poll();
    showResult(query_->result());
    if (!result_.running) {
      poll_.stop();
      query_.reset();
    }
  });
  simulated_.setSingleShot(true);
  connect(&simulated_, &QTimer::timeout, this, [this] {
    auto result = fixture_.result(readId_);
    result.elapsed_ms = elapsed_.elapsed();
    showResult(result);
  });
}
Bridge::~Bridge() { cancelRead(); }
void Bridge::clearMessage() {
  message_.clear();
  emit messageChanged();
}
void Bridge::fail(const std::exception &error) {
  message_ = safe(error.what());
  emit messageChanged();
}
void Bridge::selectArea(int index) {
  if (index < 0 || index > 6)
    return;
  leaveGuide();
  section_ = index;
  emit sectionChanged();
}
void Bridge::leaveGuide() {
  cancelRead();
  cancelReview();
  guide_.clear();
  draft_.clear();
  observation_.clear();
  readId_.clear();
  clearMessage();
  emit guideChanged();
  emit observationChanged();
}
bool Bridge::openAction(const QString &id) {
  try {
    const auto &a = action(id.toStdString());
    if (guide_.isEmpty() && section_ == 0) {
      if (a.category == "charge" || a.category == "battery" ||
          a.category == "wake")
        section_ = 1;
      else if (a.category == "desktop" || a.category == "display" ||
               a.category == "lights")
        section_ = 2;
      else if (a.category == "usb" || a.category == "network" ||
               a.id.starts_with("nfc."))
        section_ = 3;
      else if (a.category == "media" || a.category == "sensors")
        section_ = 4;
      else if (a.category == "updates")
        section_ = 5;
      emit sectionChanged();
    }
    leaveGuide();
    draft_ = defaults(a);
    QString details = q(a.description.en);
    QVariantList parameters;
    for (const auto &p : a.parameters) {
      details += "\n\n" + q(p.title.en) + ": " + q(p.description.en);
      if (p.kind == Kind::integer)
        details += QString("\nAllowed range: %1–%2")
                       .arg(qulonglong(p.minimum))
                       .arg(qulonglong(p.maximum));
      QVariantList choices;
      for (const auto &c : p.choices) {
        details += "\n" + q(c.title.en) + " — " + q(c.description.en);
        QString help = q(c.description.en);
        if (a.id == "charge.mode" && p.id == "mode") {
          if (c.value == "firmware")
            help = "Let firmware manage charging.";
          else if (c.value == "charge80")
            help = "Request an 80% charge cap without forced discharge.";
          else if (c.value == "hold70-80")
            help = "Request a 70–80% charging band for daily use.";
          else if (c.value == "hold80-90")
            help = "Request an 80–90% charging band.";
          else if (c.value == "slow")
            help = "Request the provider's supported slow-charge limit.";
          else
            help = "Use the charging provider's reviewed automatic limits.";
        }
        choices << QVariantMap{{"title", q(c.title.en)},
                               {"value", q(c.value)},
                               {"description", help}};
      }
      parameters << QVariantMap{
          {"key", q(p.id)},
          {"title", q(p.title.en)},
          {"description", q(p.description.en)},
          {"value", q(p.initial)},
          {"choices", choices},
          {"integer", p.kind == Kind::integer && p.maximum <= INT_MAX},
          {"minimum", int(std::min<std::uint64_t>(p.minimum, INT_MAX))},
          {"maximum", int(std::min<std::uint64_t>(p.maximum, INT_MAX))}};
    }
    guide_ = {{"id", id},
              {"title", friendlyTitle(a)},
              {"description", summary(a)},
              {"details", details},
              {"effect", q(effect_name(a.effect))},
              {"access", q(access_name(a.access))},
              {"parameters", parameters},
              {"read", a.effect == Effect::read},
              {"capture", a.presentation == Presentation::capture},
              {"inspect", !a.inspect.empty()},
              {"preview", !a.preview.empty()}};
    emit guideChanged();
    return true;
  } catch (const std::exception &error) {
    fail(error);
    return false;
  }
}
void Bridge::setDraft(const QString &key, const QString &value) {
  if (guide_.isEmpty() || !draft_.contains(key.toStdString()) ||
      value.size() > 4096)
    return;
  // Intermediate text is a draft. plan() validates every field before any read,
  // review or handoff. QML cannot supply executable arguments.
  cancelReview();
  draft_[key.toStdString()] = value.toStdString();
}
void Bridge::startRead(const Plan &request) {
  if (request.effect != Effect::read ||
      request.presentation != Presentation::capture)
    throw std::runtime_error(
        "This observation requires reviewed terminal access.");
  cancelRead();
  clearMessage();
  readId_ = request.action_id;
  readValues_ = request.values;
  elapsed_.start();
  Result result;
  result.action_id = readId_;
  result.running = true;
  showResult(result);
  if (fixture_.enabled)
    simulated_.start(fixture_.delay(readId_));
  else {
    try {
      query_ = std::make_unique<Query>(readId_, readValues_);
      poll_.start();
    } catch (const std::exception &error) {
      result.running = false;
      result.exit_status = 69;
      result.error = error.what();
      showResult(result);
    }
  }
}
void Bridge::submit() {
  clearMessage();
  try {
    const auto request =
        plan(guide_.value("id").toString().toStdString(), draft_);
    if (request.effect == Effect::read &&
        request.presentation == Presentation::capture)
      startRead(request);
    else
      prepare(request);
  } catch (const std::exception &error) {
    fail(error);
  }
}
void Bridge::readRelated(const QString &relation) {
  try {
    const auto &a = action(guide_.value("id").toString().toStdString());
    const auto id = relation == "inspect"   ? a.inspect
                    : relation == "preview" ? a.preview
                                            : "";
    if (id.empty())
      throw std::runtime_error("This related observation is unavailable.");
    auto values = defaults(action(id));
    for (auto &[key, value] : values)
      if (draft_.contains(key))
        value = draft_.at(key);
    const auto request = plan(id, values);
    if (request.effect == Effect::read &&
        request.presentation == Presentation::capture)
      startRead(request);
    else
      prepare(request);
  } catch (const std::exception &error) {
    fail(error);
  }
}
void Bridge::refresh() {
  if (readId_.empty() || result_.running)
    return;
  try {
    startRead(plan(readId_, readValues_));
  } catch (const std::exception &error) {
    fail(error);
  }
}
void Bridge::cancelRead() {
  poll_.stop();
  simulated_.stop();
  if (query_) {
    query_->cancel();
    query_.reset();
  }
  if (result_.running) {
    auto result = result_;
    result.running = false;
    result.cancelled = true;
    showResult(result);
  }
}
void Bridge::showResult(const Result &result) {
  result_ = result;
  QVariantList rows;
  unsigned count = 0;
  std::function<void(const QString &, const QJsonValue &, unsigned)> add;
  add = [&](const QString &key, const QJsonValue &value, unsigned depth) {
    if (count++ >= 2000)
      return;
    QString text;
    if (value.isNull() || value.isUndefined())
      text = "Unknown (not reported)";
    else if (value.isBool())
      text = value.toBool() ? "true" : "false";
    else if (value.isDouble())
      text = QString::number(value.toDouble(), 'g', 16);
    else if (value.isString())
      text = safe(value.toString().toStdString());
    else
      text = value.isArray()
                 ? QString::number(value.toArray().size()) + " entries"
                 : QString::number(value.toObject().size()) + " fields";
    rows << QVariantMap{{"key", safe(key.toStdString())},
                        {"value", text},
                        {"depth", int(depth)}};
    if (depth >= 16)
      return;
    if (value.isObject()) {
      const auto object = value.toObject();
      for (auto it = object.begin(); it != object.end() && count < 2000; ++it)
        add(it.key(), it.value(), depth + 1);
    } else if (value.isArray()) {
      const auto array = value.toArray();
      for (int i = 0; i < array.size() && count < 2000; ++i)
        add(QString::number(i), array[i], depth + 1);
    }
  };
  QString state = result.running            ? "Reading…"
                  : result.cancelled        ? "Read cancelled"
                  : result.timed_out        ? "Read timed out"
                  : result.exit_status == 0 ? "Read complete"
                                            : "Information unavailable";
  QString notice;
  if (result.cancelled)
    notice = "Read cancelled. Output may be partial.";
  else if (result.timed_out)
    notice = "The read timed out. Output may be partial.";
  else if (!result.running && result.exit_status != 0)
    notice = "The provider could not report this information. See the error "
             "details.";
  else if (!result.running && result.output.empty())
    notice = "The provider returned no information.";
  const auto document =
      QJsonDocument::fromJson(QByteArray::fromStdString(result.output));
  if (document.isObject()) {
    const auto object = document.object();
    if (!result.running && !result.cancelled && result.exit_status == 0 &&
        object.contains("available") && object.value("available").isBool() &&
        !object.value("available").toBool())
      notice = "This provider is unavailable in the current session. No device "
               "values can be shown.";
    for (auto it = object.begin(); it != object.end(); ++it)
      add(it.key(), it.value(), 0);
  } else if (document.isArray())
    add("Result", document.array(), 0);
  else if (!result.output.empty())
    add("Text result", QString::fromStdString(result.output), 0);
  if (result.truncated)
    notice += " Output was truncated; inspect the bounded raw result.";
  if (count >= 2000)
    notice += " Showing the first 2,000 values.";
  observation_ = {{"title", friendlyTitle(action(result.action_id))},
                  {"running", result.running},
                  {"state", state},
                  {"notice", notice},
                  {"failed", result.exit_status != 0 && !result.running &&
                                 !result.cancelled},
                  {"rows", rows},
                  {"raw", safe(result.output)},
                  {"error", safe(result.error)},
                  {"exitStatus", result.exit_status},
                  {"cancelled", result.cancelled},
                  {"stamp", QDateTime::currentDateTime().toString("HH:mm:ss")}};
  emit observationChanged();
}
void Bridge::prepare(const Plan &request) {
  const auto &a = action(request.action_id);
  reviewed_ = request;
  QVariantList values;
  for (const auto &p : a.parameters) {
    QString value = safe(request.values.at(p.id));
    for (const auto &choice : p.choices)
      if (choice.value == request.values.at(p.id))
        value = q(choice.title.en);
    values << QVariantMap{{"title", q(p.title.en)}, {"value", value}};
  }
  const bool blocked =
      fixture_.enabled || (readOnly_ && request.effect != Effect::read);
  QString warning;
  if (request.effect == Effect::maintenance)
    warning = "Maintenance can modify storage or prepare reboot. Preserve "
              "backups and the documented recovery route. Existing backend "
              "approvals remain required.";
  if (request.effect == Effect::publish)
    warning = "This publishes externally. Review the generated summary and "
              "exact repository before continuing.";
  if (request.access == Access::root)
    warning +=
        " Administrator authentication may be requested in the terminal.";
  review_ = {
      {"title", friendlyTitle(a)},
      {"description", summary(a)},
      {"values", values},
      {"token", q(uke::frontend::required_confirmation(request))},
      {"blocked", blocked},
      {"reason",
       fixture_.enabled
           ? "Simulation: all real commands and terminal handoffs are blocked."
           : "Read-only session: this change is blocked."},
      {"warning", warning},
      {"command", safe(display_command(request.argv))}};
  emit reviewChanged();
}
void Bridge::cancelReview() {
  reviewed_.reset();
  review_.clear();
  emit reviewChanged();
}
bool Bridge::confirmReview(bool consent, const QString &token) {
  if (!reviewed_ || !consent || fixture_.enabled ||
      (readOnly_ && reviewed_->effect != Effect::read) ||
      token != q(uke::frontend::required_confirmation(*reviewed_)))
    return false;
  try {
    const auto request = plan(reviewed_->action_id, reviewed_->values);
    cancelReview();
    const auto result =
        uke::frontend::launch_terminal(request.action_id, request.values, true);
    message_ = result.launched
                   ? "Continue in the terminal and inspect its final result. "
                     "Opening it does not establish device success."
                   : safe(result.error);
    emit messageChanged();
    return result.launched;
  } catch (const std::exception &error) {
    fail(error);
    return false;
  }
}
} // namespace uq
