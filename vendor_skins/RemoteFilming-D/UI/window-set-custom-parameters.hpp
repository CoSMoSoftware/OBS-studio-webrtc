#pragma once

#include <QWizard>
#include <QPointer>
#include <QFormLayout>
#include <QWizardPage>

#include <condition_variable>
#include <utility>
#include <thread>
#include <memory>
#include <vector>
#include <string>
#include <mutex>

class QComboBox;

class Ui_SetCustomParametersPage;

class SetCustomParametersPage;

class SetCustomParameters : public QWizard {
  Q_OBJECT

  friend class SetCustomParametersPage;

  SetCustomParametersPage *page_ = nullptr;

  virtual void done(int result) override;

  void SaveCustomParameters();
  void SaveSettings();

public:
  SetCustomParameters(QWidget *parent);
  ~SetCustomParameters();
};

class SetCustomParametersPage : public QWizardPage {
  Q_OBJECT

  friend class SetCustomParameters;

  Ui_SetCustomParametersPage *ui;

  std::string stream_name_;
  std::string publishing_token_;
  std::string fps_;

private slots:
  void on_stream_name_textChanged();
  void on_publishing_token_textChanged();
  void on_video_frame_rate_currentIndexChanged();

public:
  SetCustomParametersPage(QWidget* parent = nullptr);
  ~SetCustomParametersPage();

  virtual bool validatePage() override;
};