#include <iostream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), ui(new Ui::grep) {
  ui->setupUi(this);
  ui->stopButton->setEnabled(false);
  connect(ui->fileEdit,
          &QLineEdit::textChanged,
          this,
          [this](QString const& new_file) {
            bg_thread.set_file(new_file);
          });
  connect(ui->directoryBrowseButton, &QPushButton::clicked, this, [this] {
    ui->fileEdit->setText(QFileDialog::getExistingDirectory(this,
            tr("Select directory")));
  });
  connect(ui->fileBrowseButton, &QPushButton::clicked, this, [this] {
    ui->fileEdit->setText(QFileDialog::getOpenFileName(this,
            tr("Select file")));
  });
  connect(ui->patternEdit,
          &QLineEdit::textChanged,
          this,
          [this](QString const& new_pattern) {
            bg_thread.set_pattern(new_pattern);
          });
  connect(ui->searchButton, &QPushButton::clicked, this, [this] {
    ui->searchButton->setEnabled(false);
    ui->stopButton->setEnabled(true);
    bg_thread.start();
  });
  connect(ui->stopButton, &QPushButton::clicked, this, [this] {
    ui->searchButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    bg_thread.stop();
  });
  connect(&bg_thread, &background_thread::result_changed, this, [this] {
    auto result = bg_thread.get_result();
    auto shown = ui->listWidget->count();
    if (result.matches.empty()) {
      ui->listWidget->clear();
    }
    bool correct = (QDir(result.file).exists() || QFile(result.file).exists());
    QString info;
    if (correct) {
        for (size_t i = size_t(shown); i < result.matches.size(); ++i) {
          ui->listWidget->addItem(result.matches[i]);
        }
        info = QString("Found %1 occurrences, completed %2 files").arg(
            QString::number(result.found_matches),
            QString::number(result.completed_files));
    } else {
        info = "No such file or directory";
    }
    if (result.finished) {
      if (correct)
          info += " and still way to go";
    } else {
      ui->searchButton->setEnabled(true);
      ui->stopButton->setEnabled(false);
    }
    if (result.trouble)
        info += QString("\nError occured while handling %1").arg(result.bad_file);
    ui->infoBrowser->setText(info);
  });
}

MainWindow::~MainWindow() {
  delete ui;
}
