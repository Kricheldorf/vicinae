#include "wlr-clipboard-server.hpp"
#include "pid-file/pid-file.hpp"
#include "proto/wlr-clipboard.pb.h"
#include "services/clipboard/clipboard-server.hpp"
#include "utils/environment.hpp"
#include <QtCore>
#include <QApplication>
#include <netinet/in.h>
#include <qlogging.h>
#include <qprocess.h>
#include <qdebug.h>
#include <qresource.h>
#include <qstringview.h>

bool WlrClipboardServer::isAlive() const { return process->isOpen(); }

bool WlrClipboardServer::isActivatable() const {
  return Environment::isWaylandSession() && !Environment::isGnomeEnvironment();
}

void WlrClipboardServer::handleMessage(const proto::ext::wlrclip::Selection &sel) {
  ClipboardSelection cs;

  cs.offers.reserve(sel.offers().size());

  for (const auto &offer : sel.offers()) {
    cs.offers.push_back({offer.mime_type().c_str(), QByteArray::fromStdString(offer.data())});
  }

  emit selectionAdded(cs);
}

void WlrClipboardServer::handleExit(int code, QProcess::ExitStatus status) {}

QString WlrClipboardServer::id() const { return "wlr-clipboard"; }

int WlrClipboardServer::activationPriority() const { return 1; }

bool WlrClipboardServer::start() {
  PidFile pidFile("wlr-clip");
  int maxWaitForStart = 5000;

  if (pidFile.exists() && pidFile.kill()) { qInfo() << "Killed existing wlr-clip instance"; }

  process = new QProcess;

  process->start(WLR_CLIP_BIN, {});

  if (!process->waitForStarted(maxWaitForStart)) {
    qCritical() << "Failed to start:" << WLR_CLIP_BIN << process->errorString();
    return false;
  }

  pidFile.write(process->processId());

  connect(process, &QProcess::readyReadStandardOutput, this, &WlrClipboardServer::handleRead);
  connect(process, &QProcess::readyReadStandardError, this, &WlrClipboardServer::handleReadError);
  connect(process, &QProcess::finished, this, &WlrClipboardServer::handleExit);

  return process;
}

void WlrClipboardServer::handleReadError() { QTextStream(stderr) << process->readAllStandardError(); }

void WlrClipboardServer::handleRead() {
  auto array = process->readAllStandardOutput();
  auto _buf = array.constData();

  _message.insert(_message.end(), _buf, _buf + array.size());

  if (_messageLength == 0 && _message.size() > sizeof(uint32_t)) {
    _messageLength = ntohl(*reinterpret_cast<uint32_t *>(_message.data()));
    _message.erase(_message.begin(), _message.begin() + sizeof(uint32_t));
  }

  if (_message.size() >= _messageLength) {
    std::string data(_message.begin(), _message.begin() + _messageLength);
    proto::ext::wlrclip::Selection selection;

    if (!selection.ParseFromString(data)) {
      qWarning() << "Failed to parse selection";
    } else {
      handleMessage(selection);
    }

    _message.erase(_message.begin(), _message.begin() + _messageLength);
    _messageLength = 0;
  }
}

WlrClipboardServer::WlrClipboardServer() {}
