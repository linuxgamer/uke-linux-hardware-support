// SPDX-License-Identifier: MIT
#pragma once
#include "model-data.hpp"
#include <optional>
namespace uq {
class Bridge : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool simulation READ simulation CONSTANT)
  Q_PROPERTY(bool readOnly READ readOnly CONSTANT)
  Q_PROPERTY(QVariantList areas READ areas CONSTANT)
  Q_PROPERTY(int section READ section NOTIFY sectionChanged)
  Q_PROPERTY(QVariantList groups READ groups NOTIFY sectionChanged)
  Q_PROPERTY(QVariantMap guide READ guide NOTIFY guideChanged)
  Q_PROPERTY(QVariantMap observation READ observation NOTIFY observationChanged)
  Q_PROPERTY(QVariantMap review READ review NOTIFY reviewChanged)
  Q_PROPERTY(QString message READ message NOTIFY messageChanged)
  Fixture fixture_;
  bool readOnly_;
  int section_ = 0;
  QVariantMap guide_, observation_, review_;
  QString message_;
  Values draft_, readValues_;
  std::string readId_;
  std::optional<Plan> reviewed_;
  std::unique_ptr<Query> query_;
  QTimer poll_, simulated_;
  QElapsedTimer elapsed_;
  Result result_;
  void showResult(const Result &result);
  void startRead(const Plan &request);
  void prepare(const Plan &request);
  void fail(const std::exception &error);

public:
  explicit Bridge(Fixture fixture, bool readOnly, QObject *parent = nullptr);
  ~Bridge() override;
  bool simulation() const { return fixture_.enabled; }
  bool readOnly() const { return readOnly_; }
  int section() const { return section_; }
  QVariantList areas() const;
  QVariantList groups() const;
  QVariantMap guide() const { return guide_; }
  QVariantMap observation() const { return observation_; }
  QVariantMap review() const { return review_; }
  QString message() const { return message_; }
  Q_INVOKABLE QVariantList search(const QString &text) const;
  Q_INVOKABLE bool openAction(const QString &id);
  Q_INVOKABLE void selectArea(int index);
  Q_INVOKABLE void leaveGuide();
  Q_INVOKABLE void setDraft(const QString &key, const QString &value);
  Q_INVOKABLE void submit();
  Q_INVOKABLE void readRelated(const QString &relation);
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void cancelRead();
  Q_INVOKABLE void cancelReview();
  Q_INVOKABLE bool confirmReview(bool consent, const QString &token);
  Q_INVOKABLE void clearMessage();
signals:
  void sectionChanged();
  void guideChanged();
  void observationChanged();
  void reviewChanged();
  void messageChanged();
};
} // namespace uq
