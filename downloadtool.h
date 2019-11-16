#ifndef DOWNLOADTOOL_H
#define DOWNLOADTOOL_H

#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QTime>
#include <QUrl>
#include <QWidget>

#include "general.h"

const int kProBarMAX = 1000000;
const double kMb = 1024 * 1024.0;

QT_BEGIN_NAMESPACE
namespace Ui {
class DownloadTool;
}
QT_END_NAMESPACE

class DownloadTool : public QWidget {
  Q_OBJECT

 public:
  DownloadTool(QWidget *parent = nullptr);
  ~DownloadTool();

 protected:
  void closeEvent(QCloseEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  QFile *OpenFileForWrite(const QString &file_name);
  void StartRequest(const QUrl &requested_url);
  void PauseDownload();
  void ContinueDownload(const QUrl &url);
  void FinishDownload();

 private slots:
  void SlotNetworkFinished();
  void SlotNetworkReadyRead();
  void SlotNetworkReplyProgress(qint64 bytes_received, qint64 bytes_total);
  void SlotUpdateNetSpeed();

 private slots:
  void on_pBtn_download_clicked();

  void on_pBtn_quit_clicked();

  void on_pBtn_pause_clicked();

  void on_pBtn_choosePath_clicked();

 private:
  Ui::DownloadTool *ui; /*!< TODO: describe */

  QString download_directory_; /*!< 下载路径 */
  QString file_name_;
  QFile *file_; /*!< TODO: describe */
  QUrl url_;    /*!< TODO: describe */
  QNetworkAccessManager network_access_manager_;
  QNetworkReply *network_reply_;

  bool request_aborted_; /*!< http请求失败标志位 */

  qint64 cur_received_;  /*!< 本次下载当前请求接收到的 */
  qint64 last_received_; /*!< 本次下载上一次请求接收到的 */
  qint64 total_;         /*!< 本次下载的文件大小 */
  QTime time_zero_;

  bool pause_;                 /*!< 暂停下载 */
  qint64 breakpoint_received_; /*!< 本次下载暂停时已经接收到的 */
  bool finished_;              /*!< 本次下载是否已经完成 */
};
#endif  // DOWNLOADTOOL_H
