#include "downloadtool.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>

#include "ui_downloadtool.h"

DownloadTool::DownloadTool(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::DownloadTool),
      file_(nullptr),
      network_reply_(nullptr),
      request_aborted_(false),
      cur_received_(0),
      last_received_(0),
      total_(0),
      time_zero_(0, 0, 0),
      pause_(false),
      breakpoint_received_(0),
      finished_(true) {
  ui->setupUi(this);

  QIcon icon(":/image/download.png");
  setWindowIcon(icon);

  // 设置最大框不可用
  setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

  ui->linEdt_url->setClearButtonEnabled(true);

  download_directory_ =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  ui->linEdt_saveTo->setText(download_directory_);

  ui->proBar->setValue(0);
  // ui->proBar->hide();
  ui->label_fileSize->setText("");
  ui->label_receivedSize->setText("");
  ui->label_netSpeed->setText("");
  ui->label_netSpeed->hide();
  ui->label_needTime->setText("");
  ui->pBtn_download->setDefault(true);
  ui->pBtn_pause->setEnabled(false);

  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &DownloadTool::SlotUpdateNetSpeed);
  timer->start(500);
}

DownloadTool::~DownloadTool() { delete ui; }

void DownloadTool::closeEvent(QCloseEvent *event) {
  if (!finished_) {
    if (QMessageBox::question(this, tr("警告"),
                              tr("该文件正在下载，请确认是否退出..."),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::Yes) {
      if (!finished_) {
        if (!pause_) {
          PauseDownload();
        }
        if (file_) {
          file_->close();
          delete file_;
          file_ = nullptr;
        }
        QFile::remove(file_name_);
        event->accept();
      }
    } else {
      event->ignore();
    }
  }
}

void DownloadTool::resizeEvent(QResizeEvent *event) { Q_UNUSED(event) }

QFile *DownloadTool::OpenFileForWrite(const QString &file_name) {
  QScopedPointer<QFile> temp_file(new QFile(file_name));
  if (!temp_file->open(QIODevice::WriteOnly)) {
    QMessageBox::warning(this, tr("警告"),
                         tr("文件 %1 不能被保存: %2")
                             .arg(QDir::toNativeSeparators(file_name),
                                  temp_file->errorString()));
    return nullptr;
  }
  return temp_file.take();
}

void DownloadTool::StartRequest(const QUrl &requested_url) {
  url_ = requested_url;
  request_aborted_ = false;

  network_reply_ = network_access_manager_.get(QNetworkRequest(url_));
  connect(network_reply_, &QNetworkReply::finished, this,
          &DownloadTool::SlotNetworkFinished);
  connect(network_reply_, &QNetworkReply::readyRead, this,
          &DownloadTool::SlotNetworkReadyRead);
  connect(network_reply_, &QNetworkReply::downloadProgress, this,
          &DownloadTool::SlotNetworkReplyProgress);
}

void DownloadTool::PauseDownload() {
  disconnect(network_reply_, &QNetworkReply::downloadProgress, this,
             &DownloadTool::SlotNetworkReplyProgress);
  disconnect(network_reply_, &QNetworkReply::readyRead, this,
             &DownloadTool::SlotNetworkReadyRead);
  disconnect(network_reply_, &QNetworkReply::finished, this,
             &DownloadTool::SlotNetworkFinished);

  network_reply_->abort();
  network_reply_->deleteLater();
  network_reply_ = nullptr;
}

void DownloadTool::ContinueDownload(const QUrl &url) {
  QNetworkRequest request;
  request.setUrl(url);

  QString header_value = QString("bytes=%1-").arg(breakpoint_received_);
  request.setRawHeader(QByteArray("Range"), QByteArray(header_value.toUtf8()));

  network_reply_ = network_access_manager_.get(request);
  connect(network_reply_, &QNetworkReply::finished, this,
          &DownloadTool::SlotNetworkFinished);
  connect(network_reply_, &QNetworkReply::readyRead, this,
          &DownloadTool::SlotNetworkReadyRead);
  connect(network_reply_, &QNetworkReply::downloadProgress, this,
          &DownloadTool::SlotNetworkReplyProgress);
}

void DownloadTool::FinishDownload() {
  ui->pBtn_download->setEnabled(true);
  ui->pBtn_pause->setEnabled(false);
  QMessageBox::StandardButton ret = QMessageBox::information(
      this, tr("提醒"), tr("下载完成，是否打开文件所在目录？"),
      QMessageBox::Yes | QMessageBox::No);
  if (ret == QMessageBox::Yes) {
    QDesktopServices::openUrl(QUrl(download_directory_));
  }

  ui->label_fileSize->setText("");
  ui->label_receivedSize->setText("");
  ui->label_netSpeed->hide();
  ui->label_needTime->setText("");
  ui->pBtn_choosePath->setEnabled(true);

  cur_received_ = 0;
  last_received_ = 0;
  total_ = 0;
  ui->proBar->setValue(0);
  finished_ = true;
}

void DownloadTool::SlotNetworkFinished() {
  QFileInfo file_info;
  if (file_) {
    file_info.setFile(file_->fileName());
    // qOut << file_info;
    file_->close();
    delete file_;
    file_ = nullptr;
  }

  if (request_aborted_) {
    network_reply_->deleteLater();
    network_reply_ = nullptr;
    return;
  }

  if (network_reply_->error()) {
    qOut << "network reply errror";
    QFile::remove(file_info.absoluteFilePath());
    network_reply_->deleteLater();
    network_reply_ = nullptr;
    return;
  }

  network_reply_->deleteLater();
  network_reply_ = nullptr;

  FinishDownload();
}

void DownloadTool::SlotNetworkReadyRead() {
  if (!pause_) {
    if (file_) {
      file_->write(network_reply_->readAll());
    }
  }
}

void DownloadTool::SlotNetworkReplyProgress(qint64 bytes_received,
                                            qint64 bytes_total) {
  if (!pause_) {
    cur_received_ = bytes_received;
    total_ = bytes_total + breakpoint_received_;

    ui->proBar->setMaximum(kProBarMAX);
    ui->proBar->setValue(static_cast<int>(
        (bytes_received + breakpoint_received_) * kProBarMAX / total_));

    QString received_size = QString::asprintf(
        "%.3f", (bytes_received + breakpoint_received_) / kMb);
    QString total_size = QString::number(total_ / kMb, 'f', 3);

    ui->label_fileSize->setText(total_size + " MB");
    QString percentage = QString::asprintf(
        "%.2f", static_cast<double>((bytes_received + breakpoint_received_) *
                                    100.00 / total_));
    ui->label_receivedSize->setText(received_size + " MB ( " + percentage +
                                    "% )");
  }
}

void DownloadTool::SlotUpdateNetSpeed() {
  // 实时网速，单位：KB/s
  qint64 speed = qRound64((cur_received_ - last_received_) / 1024 / 0.5);
  if ((speed > 0) && (speed < 1000)) {
    ui->label_netSpeed->setText(QString::number(speed) + " KB/s");
  } else if (speed >= 1000) {
    QString temp = QString::asprintf("%.2f", speed / 1000.0);
    ui->label_netSpeed->setText(temp + " MB/s");
  } else {
    ui->label_netSpeed->setText("0 KB/s");
  }

  QTime time;
  if ((cur_received_ - last_received_) > 0) {
    // 剩余时间，单位：s
    qint64 sec = (total_ - (cur_received_ + breakpoint_received_)) /
                 (cur_received_ - last_received_) / 2;
    time = time_zero_.addSecs(static_cast<int>(sec));
    // qOut << time.toString("hh:mm:ss");
    if (time.hour() > 0) {
      ui->label_needTime->setText(QString::number(time.hour()) + " 时 " +
                                  QString::number(time.minute()) + " 分 " +
                                  QString::number(time.second()) + " 秒 ");
    } else if (time.minute() > 0) {
      ui->label_needTime->setText(QString::number(time.minute()) + " 分 " +
                                  QString::number(time.second()) + " 秒 ");
    } else {
      ui->label_needTime->setText(QString::number(time.second()) + " 秒 ");
    }
  }

  last_received_ = cur_received_;
}

void DownloadTool::on_pBtn_download_clicked() {
  const QString str = ui->linEdt_url->text().trimmed();
  if (str.isEmpty()) {
    QMessageBox::warning(this, tr("警告"), tr("地址栏不能为空..."),
                         QMessageBox::Ok);
    return;
  }

  const QUrl url = QUrl::fromUserInput(str);
  if (!url.isValid()) {
    QMessageBox::warning(
        this, tr("错误"),
        tr("无效的地址：%1 : %2").arg(str).arg(url.errorString()));
    return;
  }

  file_name_ = url.fileName();
  // qOut << file_name_;

  if (file_name_.isEmpty()) {
    QMessageBox::warning(this, tr("警告"),
                         tr("无效的地址：未包含可下载的文件..."));
    return;
  }

  QString download_directory =
      QDir::cleanPath(ui->linEdt_saveTo->text().trimmed());
  if (download_directory.isEmpty()) {
    QMessageBox::warning(this, tr("警告"), tr("下载目录不能为空..."),
                         QMessageBox::Ok);
    return;
  }

  if (!QFileInfo(download_directory).isDir()) {
    QMessageBox::warning(this, tr("警告"), tr("请选择正确的下载目录..."),
                         QMessageBox::Ok);
    return;
  }
  // 完整文件路径
  file_name_.prepend(download_directory + '/');
  // qOut << QFile::exists(file_name_);
  if (QFile::exists(file_name_)) {
    if (QMessageBox::question(
            this, tr("确认文件替换"),
            tr("目标路径中已存在名为 %1 的文件\n是否替换该文件...")
                .arg(file_name_.section('/', -1)),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) == QMessageBox::No) {
      return;
    }
    QFile::remove(file_name_);
  }

  file_ = OpenFileForWrite(file_name_);
  if (!file_) {
    return;
  }
  ui->pBtn_download->setEnabled(false);
  ui->label_fileSize->show();
  ui->label_netSpeed->show();
  ui->pBtn_pause->setEnabled(true);
  ui->pBtn_choosePath->setEnabled(false);

  finished_ = false;
  StartRequest(url);
}

void DownloadTool::on_pBtn_quit_clicked() { close(); }

void DownloadTool::on_pBtn_pause_clicked() {
  if (network_reply_) {
    breakpoint_received_ = file_->size();
    // 将文件移动到末尾
    file_->seek(file_->size());
    pause_ = true;
    PauseDownload();
    ui->pBtn_pause->setText("开始");
    ui->label_needTime->hide();
  } else {
    pause_ = false;
    ContinueDownload(url_);
    ui->pBtn_pause->setText("暂停");
    ui->label_needTime->show();
  }
}

void DownloadTool::on_pBtn_choosePath_clicked() {
  download_directory_ = QFileDialog::getExistingDirectory(
      this, tr("选择下载路径"),
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (download_directory_.isEmpty()) {
    return;
  }
  ui->linEdt_saveTo->setText(download_directory_);
}
