// SPDX-License-Identifier: MIT
#include "ui.hpp"
#if defined(UKE_CONTROL_SOURCE_HEADERS)
#include <frontend.hpp>
#elif __has_include(<uke-control/frontend.hpp>)
#include <uke-control/frontend.hpp>
#else
#include <frontend.hpp>
#endif
#include <algorithm>
#include <climits>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
namespace uq {
// KMessageWidget fixes its height on show/layout changes. Recalculate its
// native height-for-width after a resize so wrapped text remains visible when
// narrowing.
class NativeMessageWidget : public KMessageWidget {
public:
  using KMessageWidget::KMessageWidget;

protected:
  void resizeEvent(QResizeEvent *event) override {
    KMessageWidget::resizeEvent(event);
    if (event->oldSize().width() != event->size().width())
      QTimer::singleShot(0, this, [this] {
        const int wanted = heightForWidth(width());
        if (wordWrap() && wanted > 0 && wanted != height())
          setFixedHeight(wanted);
      });
  }
};
static QString q(const std::string &s) {
  return QString::fromUtf8(s.data(), int(s.size()));
}
static QLabel *label(const QString &text, QWidget *parent = nullptr) {
  auto *l = new QLabel(text, parent);
  l->setTextFormat(Qt::PlainText);
  l->setWordWrap(true);
  l->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  l->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return l;
}
static QLabel *heading(const QString &text, int size) {
  auto *l = label(text);
  auto font = l->font();
  font.setPointSizeF(QApplication::font().pointSizeF() + (size >= 18 ? 3 : 1));
  font.setBold(true);
  l->setFont(font);
  return l;
}
static QPushButton *button(const QString &text, const QString &icon = {}) {
  auto *b = new QPushButton(QIcon::fromTheme(icon), text);
  b->setMinimumHeight(36);
  return b;
}
static QWidget *details(const Action &a, const Plan *request = nullptr) {
  auto *widget = new QWidget;
  auto *layout = new QVBoxLayout(widget);
  layout->setContentsMargins(0, 0, 0, 0);
  auto *toggle = new QToolButton;
  toggle->setText("Details");
  toggle->setObjectName("detailsToggle");
  toggle->setCheckable(true);
  toggle->setArrowType(Qt::RightArrow);
  toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toggle->setMinimumHeight(36);
  layout->addWidget(toggle, 0, Qt::AlignLeft);
  auto *content = new QWidget;
  auto *body = new QVBoxLayout(content);
  body->setContentsMargins(12, 4, 0, 8);
  body->addWidget(label(q(a.description.en)));
  body->addWidget(label("Action: " + q(a.id) +
                        "\nEffect: " + q(effect_name(a.effect)) +
                        "\nAccess: " + q(access_name(a.access))));
  for (const auto &p : a.parameters) {
    body->addWidget(label(q(p.title.en) + ": " + q(p.description.en)));
    for (const auto &c : p.choices)
      body->addWidget(label(q(c.title.en) + " — " + q(c.description.en)));
  }
  if (request) {
    auto *exact = new QPlainTextEdit(safe(display_command(request->argv)));
    exact->setReadOnly(true);
    exact->setObjectName("validatedCommand");
    exact->setMaximumHeight(100);
    body->addWidget(exact);
  }
  content->hide();
  layout->addWidget(content);
  QObject::connect(
      toggle, &QToolButton::toggled, content, [toggle, content](bool expanded) {
        content->setVisible(expanded);
        toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
      });
  return widget;
}
Observation::Observation(Fixture fixture, QWidget *parent)
    : QWidget(parent), fixture_(std::move(fixture)) {
  setObjectName("observation");
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  notice_ = new NativeMessageWidget("No information has been read yet.");
  notice_->setObjectName("observationNotice");
  notice_->setMessageType(KMessageWidget::Information);
  notice_->setCloseButtonVisible(false);
  notice_->setWordWrap(true);
  layout->addWidget(notice_);
  auto *line = new QHBoxLayout;
  state_ = label("Not read");
  state_->setObjectName("observationState");
  line->addWidget(state_, 1);
  refresh_ = button("Refresh read", "view-refresh");
  refresh_->setObjectName("refreshRead");
  refresh_->setEnabled(false);
  stop_ = button("Stop", "process-stop");
  stop_->setObjectName("stopRead");
  stop_->setEnabled(false);
  line->addWidget(refresh_);
  line->addWidget(stop_);
  layout->addLayout(line);
  stamp_ = label("No observation loaded.");
  layout->addWidget(stamp_);
  auto *tabs = new QTabWidget;
  tree_ = new QTreeWidget;
  tree_->setObjectName("observationTree");
  tree_->setHeaderLabels({"Observation", "Value / availability"});
  tree_->setAlternatingRowColors(true);
  tree_->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  tree_->header()->setStretchLastSection(true);
  tree_->setMinimumHeight(230);
  raw_ = new QPlainTextEdit;
  raw_->setObjectName("rawOutput");
  raw_->setReadOnly(true);
  error_ = new QPlainTextEdit;
  error_->setObjectName("errorOutput");
  error_->setReadOnly(true);
  tabs->addTab(tree_, "Values");
  tabs->addTab(raw_, "Raw output");
  tabs->addTab(error_, "Errors");
  layout->addWidget(tabs, 1);
  connect(refresh_, &QPushButton::clicked, this,
          [this] { start(id_.toStdString(), values_); });
  connect(stop_, &QPushButton::clicked, this, &Observation::cancel);
  connect(&poll_, &QTimer::timeout, this, [this] {
    if (!query_)
      return;
    query_->poll();
    showResult(query_->result());
    if (!query_->result().running) {
      poll_.stop();
      query_.reset();
    }
  });
  simulated_.setSingleShot(true);
  connect(&simulated_, &QTimer::timeout, this, [this] {
    auto r = fixture_.result(id_.toStdString());
    r.elapsed_ms = elapsed_.elapsed();
    showResult(r);
  });
}
Observation::~Observation() { cancel(); }
bool Observation::running() const { return result_.running; }
void Observation::start(const std::string &id, const Values &values) {
  cancel();
  id_ = q(id);
  values_ = values;
  tree_->clear();
  raw_->clear();
  error_->clear();
  elapsed_.start();
  try {
    const auto request = plan(id, values);
    if (request.effect != Effect::read ||
        request.presentation != Presentation::capture)
      throw std::runtime_error(
          "This observation requires a reviewed foreground terminal");
    Result r;
    r.action_id = id;
    r.running = true;
    showResult(r);
    if (fixture_.enabled)
      simulated_.start(fixture_.delay(id));
    else {
      query_ = std::make_unique<Query>(id, values);
      poll_.start(40);
    }
  } catch (const std::exception &e) {
    Result r;
    r.action_id = id;
    r.error = e.what();
    r.exit_status = 69;
    showResult(r);
  }
}
void Observation::cancel() {
  simulated_.stop();
  poll_.stop();
  if (query_) {
    query_->cancel();
    query_.reset();
  }
  if (result_.running) {
    auto r = result_;
    r.running = false;
    r.cancelled = true;
    r.elapsed_ms = elapsed_.isValid() ? elapsed_.elapsed() : 0;
    showResult(r);
  }
}
void Observation::addValue(QTreeWidgetItem *parent, const QString &key,
                           const QJsonValue &value, unsigned depth) {
  if (displayedValues_++ >= 2000)
    return;
  auto *item =
      parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree_);
  item->setText(0, key);
  if (depth >= 16 && (value.isObject() || value.isArray())) {
    item->setText(1, "Nested data; inspect Raw output");
    return;
  }
  if (value.isObject()) {
    const auto object = value.toObject();
    item->setText(1, object.isEmpty()
                         ? "Empty object"
                         : QString::number(object.size()) + " fields");
    for (auto it = object.begin();
         it != object.end() && displayedValues_ < 2000; ++it)
      addValue(item, safe(it.key().toStdString()), it.value(), depth + 1);
  } else if (value.isArray()) {
    const auto a = value.toArray();
    item->setText(1, QString::number(a.size()) + " entries");
    for (int i = 0; i < a.size() && displayedValues_ < 2000; ++i)
      addValue(item, QString::number(i), a.at(i), depth + 1);
  } else if (value.isNull() || value.isUndefined())
    item->setText(1, "Unknown (not reported)");
  else if (value.isBool())
    item->setText(1, value.toBool() ? "true" : "false");
  else if (value.isDouble())
    item->setText(1, QString::number(value.toDouble(), 'g', 16));
  else
    item->setText(1, safe(value.toString().toStdString()));
}
void Observation::showResult(const Result &r) {
  result_ = r;
  stop_->setEnabled(r.running);
  refresh_->setEnabled(!r.running && !id_.isEmpty());
  QString status =
      r.running     ? "Reading…"
      : r.cancelled ? "Cancelled · output may be partial"
      : r.timed_out ? "Timed out · output may be partial"
      : r.exit_status == 0
          ? "Read complete"
          : QString("Unavailable / failed · exit %1").arg(r.exit_status);
  if (r.truncated)
    status += " · output truncated";
  if (fixture_.enabled)
    status.prepend("SIMULATION · ");
  state_->setText(status);
  notice_->setMessageType(r.exit_status != 0 && !r.running && !r.cancelled
                              ? KMessageWidget::Warning
                              : KMessageWidget::Information);
  notice_->setText(
      r.running
          ? "Reading current information… You can stop this read or go back."
      : r.cancelled ? "Read cancelled. No new request was started."
      : r.exit_status != 0
          ? "The information is unavailable. Check the messages below and "
            "retry the read when the provider is available."
      : r.output.empty()
          ? "The provider returned no information."
          : "Information read. Values describe the last observation.");
  notice_->setVisible(r.running || r.cancelled || r.exit_status != 0 ||
                      r.output.empty());
  stamp_->setText(
      r.running ? "Read in progress. Stop or leave this view to cancel."
                : QString("Last read: %1")
                      .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
  raw_->setPlainText(safe(r.output));
  error_->setPlainText(r.error.empty() ? "No stderr reported." : safe(r.error));
  tree_->clear();
  displayedValues_ = 0;
  QJsonParseError err;
  const auto doc =
      QJsonDocument::fromJson(QByteArray::fromStdString(r.output), &err);
  if (err.error == QJsonParseError::NoError) {
    if (doc.isObject()) {
      auto obj = doc.object();
      if (!r.running && !r.cancelled && r.exit_status == 0 &&
          obj.contains("available")) {
        if (obj.value("available").isBool() &&
            !obj.value("available").toBool()) {
          state_->setText(status + " · provider unavailable");
          notice_->setMessageType(KMessageWidget::Information);
          notice_->setText("This provider is not available in the current "
                           "session. No device values can be shown.");
          notice_->show();
        } else if (obj.value("available").isNull())
          state_->setText(status + " · availability unknown");
      }
      for (auto it = obj.begin(); it != obj.end(); ++it)
        addValue(nullptr, safe(it.key().toStdString()), it.value());
    } else
      addValue(nullptr, "Result", doc.array());
    tree_->expandToDepth(1);
  } else if (!r.output.empty()) {
    auto *i = new QTreeWidgetItem(tree_, {"Text result", safe(r.output)});
    i->setToolTip(1, safe(r.output));
  } else if (!r.running)
    new QTreeWidgetItem(tree_,
                        {"Availability", r.cancelled ? "Cancelled"
                                         : r.exit_status == 0
                                             ? "No output reported"
                                             : "Unavailable; inspect Errors"});
  if (displayedValues_ >= 2000)
    new QTreeWidgetItem(tree_, {"Display limit",
                                "First 2,000 values shown; inspect Raw output "
                                "for the full bounded result"});
}
struct Section {
  const char *title;
  const char *icon;
  const char *description;
};
static constexpr Section sections[] = {
    {"Status", "dialog-information",
     "See device information and current availability."},
    {"Power", "battery", "Battery, charging preferences, and wake settings."},
    {"Display & lights", "video-display",
     "Screen preferences, flashlight, and notification light."},
    {"Connections", "network-wired", "USB, Ethernet, and NFC connections."},
    {"Audio & sensors", "audio-card",
     "Audio, microphone, camera, and sensor information."},
    {"Updates", "system-software-update",
     "Check your software channel and review available updates."},
    {"Advanced", "preferences-system",
     "Additional diagnostics, recovery, and expert tools."}};
Window::Window(Fixture fixture, bool readOnly, QString styleNotice)
    : fixture_(std::move(fixture)), readOnly_(readOnly),
      styleNotice_(std::move(styleNotice)) {
  setWindowTitle("Uke Control");
  resize(1100, 760);
  setMinimumSize(480, 480);
  setWindowIcon(QIcon::fromTheme("preferences-system"));

  auto *toolbar = addToolBar("Navigation");
  toolbar->setObjectName("navigationToolbar");
  toolbar->setMovable(false);
  toolbar->setIconSize(QSize(22, 22));
  toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  navigationAction_ =
      toolbar->addAction(QIcon::fromTheme("sidebar-show"), "Settings");
  navigationAction_->setObjectName("showNavigation");
  navigationAction_->setCheckable(true);
  navigationAction_->setChecked(true);
  backAction_ = toolbar->addAction(QIcon::fromTheme("go-previous"), "Back");
  backAction_->setObjectName("goBack");
  backAction_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
  backAction_->setEnabled(false);
  connect(backAction_, &QAction::triggered, this, &Window::back);
  toolbar->addSeparator();
  auto *spacer = new QWidget;
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  toolbar->addWidget(spacer);
  auto *help = toolbar->addAction(QIcon::fromTheme("help-about"), "About");
  connect(help, &QAction::triggered, this, [this] {
    QMessageBox::about(
        this, "About Uke Control",
        "Uke Control\nNative Qt Widgets and KDE Frameworks "
        "interface.\n\nControls use the existing Uke backend. Open an area to "
        "inspect its current state before changing settings.");
  });

  sidebar_ = new QDockWidget("Settings", this);
  sidebar_->setObjectName("settingsSidebar");
  sidebar_->setFeatures(QDockWidget::NoDockWidgetFeatures);
  sidebar_->setTitleBarWidget(new QWidget);
  sidebar_->setAllowedAreas(Qt::LeftDockWidgetArea);
  sidebar_->setMinimumWidth(210);
  sidebar_->setMaximumWidth(260);
  auto *side = new QWidget;
  auto *sideLayout = new QVBoxLayout(side);
  sideLayout->setContentsMargins(0, 0, 0, 0);
  sideLayout->setSpacing(0);
  auto *searchContainer = new QWidget;
  auto *searchLayout = new QVBoxLayout(searchContainer);
  searchLayout->setContentsMargins(8, 8, 8, 8);
  settingsSearch_ = new QLineEdit;
  settingsSearch_->setObjectName("settingsSearch");
  settingsSearch_->setPlaceholderText("Filter settings areas…");
  settingsSearch_->setClearButtonEnabled(true);
  settingsSearch_->setMinimumHeight(36);
  searchLayout->addWidget(settingsSearch_);
  sideLayout->addWidget(searchContainer);
  auto *noAreas = label("No matching settings area.");
  noAreas->setObjectName("navigationEmpty");
  noAreas->setContentsMargins(10, 8, 10, 8);
  noAreas->hide();
  sideLayout->addWidget(noAreas);
  navigation_ = new QListWidget;
  navigation_->setObjectName("navigation");
  navigation_->setFrameShape(QFrame::NoFrame);
  navigation_->setIconSize(QSize(22, 22));
  navigation_->setSpacing(0);
  navigation_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  QScroller::grabGesture(navigation_->viewport(), QScroller::TouchGesture);
  for (const auto &section : sections) {
    auto *item = new QListWidgetItem(QIcon::fromTheme(section.icon),
                                     section.title, navigation_);
    item->setSizeHint(QSize(190, 44));
  }
  sideLayout->addWidget(navigation_, 1);
  sidebar_->setWidget(side);
  addDockWidget(Qt::LeftDockWidgetArea, sidebar_);
  connect(navigationAction_, &QAction::toggled, this, [this](bool visible) {
    if (!narrow_) {
      sidebar_->setVisible(visible);
      return;
    }
    if (!visible)
      return;
    QSignalBlocker blocked(navigationAction_);
    navigationAction_->setChecked(false);
    QMenu destinations(this);
    for (int i = 0; i < 7; ++i) {
      auto *item = destinations.addAction(QIcon::fromTheme(sections[i].icon),
                                          sections[i].title);
      item->setCheckable(true);
      item->setChecked(section_ == i);
      connect(item, &QAction::triggered, this, [this, i] {
        settingsSearch_->clear();
        navigation_->setCurrentRow(i);
      });
    }
    destinations.exec(mapToGlobal(
        QPoint(8, toolBarArea(findChild<QToolBar *>("navigationToolbar")) ==
                          Qt::TopToolBarArea
                      ? 40
                      : 8)));
  });
  connect(sidebar_, &QDockWidget::visibilityChanged, navigationAction_,
          &QAction::setChecked);
  connect(settingsSearch_, &QLineEdit::textChanged, this,
          [this, noAreas](const QString &text) {
            unsigned matches = 0;
            for (int i = 0; i < navigation_->count(); ++i)
              navigation_->item(i)->setHidden(
                  !navigation_->item(i)->text().contains(text,
                                                         Qt::CaseInsensitive));
            for (int i = 0; i < navigation_->count(); ++i)
              if (!navigation_->item(i)->isHidden())
                ++matches;
            noAreas->setVisible(!matches);
          });

  auto *central = new QWidget;
  contentLayout_ = new QVBoxLayout(central);
  contentLayout_->setContentsMargins(22, 18, 22, 12);
  contentLayout_->setSpacing(8);
  pageTitle_ = heading("Overview", 18);
  pageTitle_->setObjectName("pageTitle");
  contentLayout_->addWidget(pageTitle_);
  pageSubtitle_ = label("");
  pageSubtitle_->setObjectName("pageDescription");
  contentLayout_->addWidget(pageSubtitle_);
  sessionNotice_ = new NativeMessageWidget;
  sessionNotice_->setCloseButtonVisible(false);
  sessionNotice_->setWordWrap(true);
  if (fixture_.enabled) {
    sessionNotice_->setObjectName("simulationIndicator");
    sessionNotice_->setMessageType(KMessageWidget::Warning);
    sessionNotice_->setText("Simulation — sample data only. Device commands "
                            "and changes are disabled.");
  } else if (readOnly_) {
    sessionNotice_->setMessageType(KMessageWidget::Information);
    sessionNotice_->setText("Read-only session. You can inspect information; "
                            "changes are disabled.");
  } else
    sessionNotice_->hide();
  contentLayout_->addWidget(sessionNotice_);
  if (!styleNotice_.isEmpty()) {
    auto *warning = new NativeMessageWidget(styleNotice_);
    warning->setObjectName("styleNotice");
    warning->setMessageType(KMessageWidget::Warning);
    warning->setCloseButtonVisible(false);
    warning->setWordWrap(true);
    contentLayout_->addWidget(warning);
  }
  stack_ = new QStackedWidget;
  contentLayout_->addWidget(stack_, 1);
  setCentralWidget(central);
  connect(navigation_, &QListWidget::currentRowChanged, this,
          &Window::showSection);
  navigation_->setCurrentRow(0);
  auto *quit = new QAction(this);
  quit->setShortcut(QKeySequence::Quit);
  addAction(quit);
  connect(quit, &QAction::triggered, this, &Window::close);
}
void Window::updateNavigation(int width) {
  const bool compact = width < 800;
  if (compact != narrow_) {
    narrow_ = compact;
    sidebar_->setVisible(!compact);
  }
  if (form_)
    form_->setRowWrapPolicy(width < 660 ? QFormLayout::WrapAllRows
                                        : QFormLayout::DontWrapRows);
}
void Window::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  updateNavigation(event->size().width());
}
Window::~Window() {
  if (observation_)
    observation_->cancel();
}
void Window::closeEvent(QCloseEvent *event) {
  if (observation_)
    observation_->cancel();
  QMainWindow::closeEvent(event);
}
void Window::back() { showSection(section_); }
void Window::addGroup(QVBoxLayout *layout, const QString &title,
                      const QString &subtitle,
                      const std::vector<std::string> &ids) {
  auto *group = new QGroupBox(QString(title).replace("&", "&&"));
  group->setFlat(true);
  group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  group->setAlignment(Qt::AlignLeft);
  auto *rows = new QVBoxLayout(group);
  rows->setContentsMargins(12, 18, 8, 10);
  rows->setSpacing(0);
  auto *description = label(subtitle);
  description->setContentsMargins(0, 0, 0, 8);
  rows->addWidget(description);
  for (const auto &id : ids) {
    const auto &a = action(id);
    auto *row = new QToolButton;
    row->setText(friendlyTitle(a));
    row->setIcon(QIcon::fromTheme(a.effect == Effect::read ? "document-preview"
                                                           : "configure"));
    row->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    row->setAutoRaise(true);
    row->setMinimumHeight(42);
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    row->setObjectName(q(id));
    row->setToolTip(summary(a));
    rows->addWidget(row, 0, Qt::AlignLeft);
    connect(row, &QToolButton::clicked, this, [this, id] { openAction(id); });
  }
  layout->addWidget(group);
}
void Window::showSection(int index) {
  if (index < 0 || index > 6)
    return;
  section_ = index;
  backAction_->setEnabled(false);
  if (actionBar_) {
    contentLayout_->removeWidget(actionBar_);
    delete actionBar_;
    actionBar_ = nullptr;
  }
  form_ = nullptr;
  if (observation_)
    observation_->cancel();
  observation_ = nullptr;
  fields_.clear();
  if (detail_) {
    stack_->removeWidget(detail_);
    delete detail_;
    detail_ = nullptr;
  }
  if (page_) {
    stack_->removeWidget(page_);
    delete page_;
  }
  pageTitle_->setText(sections[index].title);
  pageSubtitle_->setText(sections[index].description);
  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  page_ = scroll;
  auto *content = new QWidget;
  auto *l = new QVBoxLayout(content);
  l->setContentsMargins(0, 4, 8, 12);
  l->setSpacing(18);
  if (index == 0) {
    addGroup(l, "Device at a glance",
             "Choose the information you want to read.",
             {"status", "overview.battery", "overview.workload"});
    addGroup(l, "System information", "Hardware, desktop, and storage details.",
             {"hardware.status", "overview.desktop", "overview.storage"});
  }
  if (index == 1) {
    addGroup(l, "Battery & charging",
             "Measured values and saved requests are shown separately.",
             {"battery.status", "battery.health", "charge.status",
              "charge.admission"});
    addGroup(l, "Charging requests",
             "Check availability and preview your preference before applying.",
             {"charge.mode", "charge.limit"});
    addGroup(l, "Sleep & wake",
             "Read available sleep modes and review wake preferences.",
             {"overview.sleep", "wake.status", "wake.set"});
  }
  if (index == 2) {
    addGroup(l, "Desktop",
             "Screen preferences for your current desktop session.",
             {"desktop.plasma", "desktop.rotation", "desktop.brightness"});
    addGroup(l, "Lights", "Flashlight and notification light preferences.",
             {"flashlight.status", "flashlight.state", "flashlight.level",
              "led.status", "led.color", "led.auto"});
    addGroup(l, "Display link", "Read information about a connected display.",
             {"display.bits"});
  }
  if (index == 3) {
    addGroup(l, "USB & accessories",
             "Read the current USB role and connected accessory information.",
             {"usb.status"});
    addGroup(l, "Network & NFC",
             "Read connection information and available preferences.",
             {"ethernet.status", "overview.nfc", "nfc.state"});
  }
  if (index == 4) {
    addGroup(l, "Audio & microphone",
             "Audio information and recording preferences.",
             {"overview.audio", "microphone.status", "microphone.select",
              "overview.camera"});
    addGroup(l, "Sensors",
             "Read sensor information once or for a short interval.",
             {"sensors.list", "sensors.once", "sensors.watch"});
  }
  if (index == 5) {
    addGroup(l, "Channel & update review",
             "Check available updates before making changes.",
             {"channel.status", "update.preview"});
    addGroup(l, "Apply reviewed work",
             "Inspect unfinished updates before continuing. Wait for "
             "completion before rebooting.",
             {"update.apply"});
  }
  if (index == 6) {
    auto *search = new QLineEdit;
    search->setObjectName("advancedSearch");
    search->setPlaceholderText(
        "Search all actions, descriptions or identifiers…");
    search->setMinimumHeight(40);
    l->addWidget(search);
    auto *empty = label(
        "No actions match this search. Clear the search to see all actions.");
    empty->setObjectName("advancedEmpty");
    empty->hide();
    l->addWidget(empty);
    auto *list = new QListWidget;
    list->setObjectName("advancedActions");
    list->setMinimumHeight(420);
    for (const auto &a : uke::client::actions()) {
      auto *item = new QListWidgetItem(q(a.title.en) + "\n" + q(a.id) + " · " +
                                           q(effect_name(a.effect)),
                                       list);
      item->setData(Qt::UserRole, q(a.id));
      item->setData(Qt::UserRole + 1,
                    q(a.title.en + " " + a.description.en + " " + a.id));
      item->setSizeHint(QSize(0, 60));
    }
    l->addWidget(list);
    auto *open = button("Open selected action", "go-next");
    open->setObjectName("openAdvancedAction");
    open->setEnabled(false);
    l->addWidget(open);
    connect(search, &QLineEdit::textChanged, this,
            [list, open, empty](const QString &text) {
              QListWidgetItem *first = nullptr;
              for (int i = 0; i < list->count(); ++i) {
                auto *item = list->item(i);
                item->setHidden(!item->data(Qt::UserRole + 1)
                                     .toString()
                                     .contains(text, Qt::CaseInsensitive));
                if (!item->isHidden() && !first)
                  first = item;
              }
              list->setCurrentItem(first);
              open->setEnabled(first);
              empty->setVisible(!first);
            });
    connect(list, &QListWidget::currentItemChanged, this,
            [open](QListWidgetItem *current) {
              open->setEnabled(current && !current->isHidden());
            });
    connect(open, &QPushButton::clicked, this, [this, list] {
      if (auto *item = list->currentItem())
        openAction(item->data(Qt::UserRole).toString().toStdString());
    });
    connect(list, &QListWidget::itemActivated, this,
            [this](QListWidgetItem *item) {
              openAction(item->data(Qt::UserRole).toString().toStdString());
            });
  }
  l->addStretch();
  QScroller::grabGesture(scroll->viewport(), QScroller::TouchGesture);
  scroll->setWidget(content);
  stack_->addWidget(page_);
  stack_->setCurrentWidget(page_);
}
Values Window::values() const {
  Values result;
  for (const auto &[id, w] : fields_) {
    if (auto *combo = qobject_cast<QComboBox *>(w))
      result[id] = combo->currentData().toString().toStdString();
    else if (auto *spin = qobject_cast<QSpinBox *>(w))
      result[id] = std::to_string(spin->value());
    else if (auto *edit = qobject_cast<QLineEdit *>(w))
      result[id] = edit->text().toStdString();
  }
  return result;
}
void Window::openAction(const std::string &id) {
  const auto &a = action(id);
  if (observation_)
    observation_->cancel();
  if (actionBar_) {
    contentLayout_->removeWidget(actionBar_);
    delete actionBar_;
    actionBar_ = nullptr;
  }
  form_ = nullptr;
  backAction_->setEnabled(true);
  // Keep deep links aligned with the daily navigation. Advanced searches retain
  // their context.
  if (!detail_ && section_ == 0) {
    int owner = 0;
    if (a.category == "charge" || a.category == "battery" ||
        a.category == "wake")
      owner = 1;
    else if (a.category == "display" || a.category == "desktop" ||
             a.category == "lights")
      owner = 2;
    else if (a.category == "usb" || a.category == "network" ||
             a.id.starts_with("nfc."))
      owner = 3;
    else if (a.category == "media" || a.category == "sensors")
      owner = 4;
    else if (a.category == "updates")
      owner = 5;
    if (owner) {
      QSignalBlocker blocked(navigation_);
      navigation_->setCurrentRow(owner);
      section_ = owner;
    }
  }
  actionId_ = id;
  fields_.clear();
  observation_ = nullptr;
  if (detail_) {
    stack_->removeWidget(detail_);
    delete detail_;
  }
  pageTitle_->setText(friendlyTitle(a));
  pageSubtitle_->setText(summary(a));
  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  detail_ = scroll;
  auto *content = new QWidget;
  auto *layout = new QVBoxLayout(content);
  layout->setContentsMargins(0, 4, 8, 12);
  layout->setSpacing(14);
  if (!a.parameters.empty()) {
    auto *group =
        new QGroupBox(a.effect == Effect::read ? "Options" : "Your preference");
    group->setFlat(true);
    group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    group->setAlignment(Qt::AlignLeft);
    auto *form = new QFormLayout(group);
    form_ = form;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(width() < 660 ? QFormLayout::WrapAllRows
                                         : QFormLayout::DontWrapRows);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setContentsMargins(12, 22, 12, 12);
    form->setSpacing(12);
    form->addRow(label("These are draft choices. Nothing changes until you "
                       "review and continue."));
    for (const auto &p : a.parameters) {
      QWidget *field = nullptr;
      if (p.kind == Kind::choice) {
        auto *combo = new QComboBox;
        combo->setSizeAdjustPolicy(
            QComboBox::AdjustToMinimumContentsLengthWithIcon);
        combo->setMinimumContentsLength(18);
        for (const auto &c : p.choices) {
          combo->addItem(q(c.title.en), q(c.value));
          combo->setItemData(combo->count() - 1, q(c.description.en),
                             Qt::ToolTipRole);
        }
        combo->setCurrentIndex(std::max(0, combo->findData(q(p.initial))));
        field = combo;
      } else if (p.kind == Kind::integer && p.maximum <= INT_MAX) {
        auto *spin = new QSpinBox;
        spin->setRange(int(p.minimum), int(p.maximum));
        spin->setValue(q(p.initial).toInt());
        spin->setSuffix(p.id == "percent"   ? " %"
                        : p.id == "hz"      ? " Hz"
                        : p.id == "minutes" ? " min"
                                            : "");
        field = spin;
      } else {
        auto *edit = new QLineEdit(q(p.initial));
        edit->setMaxLength(4096);
        field = edit;
      }
      field->setMinimumHeight(36);
      field->setObjectName("parameter_" + q(p.id));
      field->setAccessibleName(q(p.title.en));
      field->setToolTip(q(p.description.en));
      fields_[p.id] = field;
      auto *fieldBox = new QWidget;
      fieldBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
      auto *fl = new QVBoxLayout(fieldBox);
      fl->setContentsMargins(0, 0, 0, 0);
      fl->addWidget(field);
      if (p.kind == Kind::integer)
        fl->addWidget(label(
            QString("Allowed range: %1–%2").arg(p.minimum).arg(p.maximum)));
      else if (p.kind == Kind::color)
        fl->addWidget(label("Enter #RRGGBB or choose a color."));
      else if (p.kind != Kind::choice)
        fl->addWidget(label(q(p.description.en)));
      if (auto *combo = qobject_cast<QComboBox *>(field);
          combo && p.id == "mode" && a.category == "charge") {
        auto choiceHelp = [combo] {
          const auto value = combo->currentData().toString();
          if (value == "firmware")
            return QString("Leave charging under firmware management.");
          if (value == "charge80")
            return QString(
                "Request a charge cap of 80%, without forced discharge.");
          if (value == "hold70-80")
            return QString("Request charging within a daily 70–80% band.");
          if (value == "hold80-90")
            return QString("Request charging within an 80–90% band.");
          if (value == "slow")
            return QString(
                "Use the charging provider's supported slow-charge limit.");
          return QString(
              "Use the charging provider's reviewed automatic limits.");
        };
        auto *explanation = label(choiceHelp());
        explanation->setObjectName("choiceExplanation_" + q(p.id));
        fl->addWidget(explanation);
        connect(
            combo, &QComboBox::currentIndexChanged, explanation,
            [choiceHelp, explanation] { explanation->setText(choiceHelp()); });
      }
      if (p.kind == Kind::color) {
        auto *edit = qobject_cast<QLineEdit *>(field);
        auto *choose = button("Choose color…", "color-management");
        fl->addWidget(choose);
        connect(choose, &QPushButton::clicked, this, [this, edit] {
          const auto color = QColorDialog::getColor(QColor(edit->text()), this,
                                                    "Requested LED color");
          if (color.isValid())
            edit->setText(color.name(QColor::HexRgb));
        });
      }
      form->addRow(q(p.title.en), fieldBox);
    }
    layout->addWidget(group);
  }
  auto *related = new QHBoxLayout;
  if (!a.inspect.empty()) {
    auto *b = button("Inspect current state", "document-preview");
    b->setObjectName("inspectCurrent");
    related->addWidget(b);
    connect(b, &QPushButton::clicked, this,
            [this, id = a.inspect] { inspect(id); });
  }
  if (!a.preview.empty()) {
    auto *b = button("Preview requested values", "document-preview");
    b->setObjectName("previewRequest");
    related->addWidget(b);
    connect(b, &QPushButton::clicked, this,
            [this, id = a.preview] { inspect(id); });
  }
  related->addStretch();
  layout->addLayout(related);
  layout->addWidget(details(a));
  primary_ =
      button(a.effect == Effect::read ? (a.presentation == Presentation::capture
                                             ? "Read observation"
                                             : "Review terminal observation")
                                      : "Review request",
             a.effect == Effect::read ? "view-refresh" : "dialog-ok-apply");
  primary_->setObjectName("primaryAction");
  primary_->setText(a.effect == Effect::read
                        ? (a.presentation == Presentation::capture
                               ? "Read current information"
                               : "Review access…")
                        : "Review changes…");
  actionBar_ = new QWidget;
  auto *actionLayout = new QHBoxLayout(actionBar_);
  actionLayout->setContentsMargins(0, 8, 0, 0);
  actionLayout->addStretch();
  actionLayout->addWidget(primary_);
  contentLayout_->addWidget(actionBar_);
  connect(primary_, &QPushButton::clicked, this, &Window::submit);
  if (a.effect == Effect::read && a.presentation == Presentation::capture) {
    observation_ = new Observation(fixture_);
    layout->addWidget(observation_, 1);
  } else {
    layout->addStretch();
  }
  QScroller::grabGesture(scroll->viewport(), QScroller::TouchGesture);
  scroll->setWidget(content);
  stack_->addWidget(detail_);
  updateNavigation(width());
  if (narrow_)
    sidebar_->hide();
  stack_->setCurrentWidget(detail_);
}
void Window::inspect(const std::string &id) {
  try {
    auto v = defaults(action(id));
    const auto draft = values();
    for (auto &[key, value] : v)
      if (draft.contains(key))
        value = draft.at(key);
    const auto request = plan(id, v);
    if (request.presentation == Presentation::terminal) {
      review(request);
      return;
    }
    QDialog dialog(this);
    dialog.setObjectName("inspectionDialog");
    dialog.setWindowTitle(friendlyTitle(action(id)));
    dialog.resize(std::min(820, std::max(420, width() - 32)),
                  std::min(650, std::max(440, height() - 64)));
    dialog.setMinimumSize(420, 400);
    auto *outer = new QVBoxLayout(&dialog);
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget;
    auto *l = new QVBoxLayout(content);
    scroll->setWidget(content);
    QScroller::grabGesture(scroll->viewport(), QScroller::TouchGesture);
    outer->addWidget(scroll, 1);
    l->addWidget(label(summary(action(id))));
    auto *observation = new Observation(fixture_);
    l->addWidget(observation, 1);
    l->addWidget(details(action(id)));
    auto *close = new QDialogButtonBox(QDialogButtonBox::Close);
    close->button(QDialogButtonBox::Close)->setMinimumHeight(36);
    outer->addWidget(close);
    connect(close, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    observation->start(id, v);
    dialog.exec();
    observation->cancel();
  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Cannot inspect", safe(e.what()));
  }
}
void Window::submit() {
  try {
    const auto request = plan(actionId_, values());
    if (request.effect == Effect::read &&
        request.presentation == Presentation::capture) {
      observation_->start(actionId_, request.values);
      return;
    }
    review(request);
  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Check requested values", safe(e.what()));
  }
}
void Window::review(const Plan &request) {
  QDialog dialog(this);
  dialog.setObjectName("requestReview");
  const auto &a = action(request.action_id);
  dialog.setWindowTitle("Review · " + friendlyTitle(a));
  dialog.resize(std::min(720, std::max(420, width() - 32)),
                std::min(650, std::max(440, height() - 64)));
  dialog.setMinimumSize(420, 400);
  auto *outer = new QVBoxLayout(&dialog);
  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  auto *content = new QWidget;
  auto *l = new QVBoxLayout(content);
  scroll->setWidget(content);
  outer->addWidget(scroll, 1);
  l->setSpacing(14);
  l->addWidget(heading("Review before continuing", 18));
  l->addWidget(heading(friendlyTitle(a), 14));
  l->addWidget(label(summary(a)));
  for (const auto &p : a.parameters) {
    QString value = safe(request.values.at(p.id));
    for (const auto &c : p.choices)
      if (c.value == request.values.at(p.id))
        value = q(c.title.en);
    l->addWidget(label(q(p.title.en) + ": " + value));
  }
  if (request.access == Access::root)
    l->addWidget(label(
        "Administrator authentication may be requested in the terminal."));
  if (request.effect == Effect::maintenance)
    l->addWidget(
        label("Maintenance can modify storage or prepare reboot. Preserve "
              "backups and the documented external recovery route. Existing "
              "plan-bound backend approvals remain required."));
  if (request.effect == Effect::publish)
    l->addWidget(
        label("This request publishes externally. Review the generated summary "
              "and the exact repository before continuing."));
  l->addWidget(details(a, &request));
  QString token = q(uke::frontend::required_confirmation(request));
  QLineEdit *confirmation = nullptr;
  if (!token.isEmpty()) {
    l->addWidget(label("Type " + token + " to confirm this reviewed request:"));
    confirmation = new QLineEdit;
    confirmation->setObjectName("confirmationToken");
    confirmation->setMinimumHeight(38);
    l->addWidget(confirmation);
  }
  const auto consentText =
      request.effect == Effect::read
          ? "I reviewed this foreground observation and its access "
            "requirements."
          : "I reviewed the request, its effects and the current state.";
  auto *consent = new QCheckBox;
  consent->setAccessibleName(consentText);
  consent->setMinimumSize(40, 40);
  consent->setObjectName("reviewConsent");
  auto *consentRow = new QHBoxLayout;
  auto *consentLabel = label(consentText);
  consentLabel->setBuddy(consent);
  consentRow->addWidget(consent, 0, Qt::AlignTop);
  consentRow->addWidget(consentLabel, 1);
  l->addLayout(consentRow);
  const bool blocked =
      fixture_.enabled || (readOnly_ && request.effect != Effect::read);
  if (blocked)
    l->addWidget(label(fixture_.enabled
                           ? "SIMULATION: terminal launch and every real "
                             "command/write are blocked."
                           : "Read-only session: this change is blocked."));
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
  buttons->button(QDialogButtonBox::Cancel)->setMinimumHeight(36);
  auto *run =
      buttons->addButton("Continue in terminal", QDialogButtonBox::AcceptRole);
  run->setObjectName("launchTerminal");
  run->setMinimumHeight(40);
  run->setEnabled(false);
  l->addStretch();
  outer->addWidget(buttons);
  auto update = [=] {
    run->setEnabled(!blocked && consent->isChecked() &&
                    (!confirmation || confirmation->text() == token));
  };
  connect(consent, &QCheckBox::toggled, &dialog, update);
  if (confirmation)
    connect(confirmation, &QLineEdit::textChanged, &dialog, update);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  connect(run, &QPushButton::clicked, &dialog, &QDialog::accept);
  if (dialog.exec() != QDialog::Accepted || blocked)
    return;
  // The shared terminal adapter revalidates the ID and values. No shell command
  // is executed by this GUI.
  const auto launched =
      uke::frontend::launch_terminal(request.action_id, request.values, true);
  if (!launched.launched)
    QMessageBox::warning(this, "Cannot open terminal", safe(launched.error));
  else
    QMessageBox::information(
        this, "Continue in terminal",
        launched.error.empty()
            ? "Continue in the terminal. Check the final result there, then "
              "refresh current state here."
            : safe(launched.error));
}
} // namespace uq
