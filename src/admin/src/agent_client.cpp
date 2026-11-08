#include "agent_client.hpp"
#include <QJsonDocument>
#include <QUrl>

namespace synx::admin {

AgentClient::AgentClient(const QString& base_url, const QString& auth_token, QObject* parent)
    : QObject(parent), base_url_(base_url), auth_token_(auth_token), nam_(new QNetworkAccessManager(this)) {}

QNetworkRequest AgentClient::create_request(const QString& endpoint) const {
    QUrl url(base_url_ + endpoint);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("X-Synx-Agent-Token", auth_token_.toUtf8());
    return req;
}

void AgentClient::send_get(const QString& endpoint, std::function<void(bool, const QByteArray&, const QString&)> cb) {
    QNetworkReply* reply = nam_->get(create_request(endpoint));
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            cb(true, reply->readAll(), QString());
        } else {
            cb(false, reply->readAll(), reply->errorString());
        }
    });
}

void AgentClient::send_post(const QString& endpoint, const QByteArray& data, std::function<void(bool, const QByteArray&, const QString&)> cb) {
    QNetworkReply* reply = nam_->post(create_request(endpoint), data);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            cb(true, reply->readAll(), QString());
        } else {
            cb(false, reply->readAll(), reply->errorString());
        }
    });
}

void AgentClient::send_delete(const QString& endpoint, std::function<void(bool, const QByteArray&, const QString&)> cb) {
    QNetworkReply* reply = nam_->deleteResource(create_request(endpoint));
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            cb(true, reply->readAll(), QString());
        } else {
            cb(false, reply->readAll(), reply->errorString());
        }
    });
}

void AgentClient::get_health(JsonCallback cb) {
    send_get("/api/v1/health", [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        auto doc = QJsonDocument::fromJson(bytes);
        cb(true, doc.object(), QString());
    });
}

void AgentClient::get_metrics(JsonCallback cb) {
    send_get("/api/v1/host/metrics", [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        auto doc = QJsonDocument::fromJson(bytes);
        cb(true, doc.object(), QString());
    });
}

void AgentClient::list_instances(ArrayCallback cb) {
    send_get("/api/v1/instances", [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        auto doc = QJsonDocument::fromJson(bytes);
        cb(true, doc.array(), QString());
    });
}

void AgentClient::get_instance(const QString& id, JsonCallback cb) {
    send_get(QString("/api/v1/instances/%1").arg(id), [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        auto doc = QJsonDocument::fromJson(bytes);
        cb(true, doc.object(), QString());
    });
}

void AgentClient::create_instance(const QJsonObject& payload, JsonCallback cb) {
    QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    send_post("/api/v1/instances", data, [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) {
            auto doc = QJsonDocument::fromJson(bytes);
            QString msg = doc.object().value("error").toString(err);
            cb(false, {}, msg);
            return;
        }
        auto doc = QJsonDocument::fromJson(bytes);
        cb(true, doc.object(), QString());
    });
}

void AgentClient::delete_instance(const QString& id, JsonCallback cb) {
    send_delete(QString("/api/v1/instances/%1").arg(id), [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        auto doc = QJsonDocument::fromJson(bytes);
        cb(true, doc.object(), QString());
    });
}

void AgentClient::send_power_action(const QString& id, const QString& action, JsonCallback cb) {
    QJsonObject body{{"action", action}};
    send_post(QString("/api/v1/instances/%1/power").arg(id), QJsonDocument(body).toJson(QJsonDocument::Compact), [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) {
            auto doc = QJsonDocument::fromJson(bytes);
            QString msg = doc.object().value("error").toString(err);
            cb(false, {}, msg);
            return;
        }
        cb(true, QJsonDocument::fromJson(bytes).object(), QString());
    });
}

void AgentClient::send_command(const QString& id, const QString& command, JsonCallback cb) {
    QJsonObject body{{"command", command}};
    send_post(QString("/api/v1/instances/%1/command").arg(id), QJsonDocument(body).toJson(QJsonDocument::Compact), [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        cb(true, QJsonDocument::fromJson(bytes).object(), QString());
    });
}

void AgentClient::get_console_lines(const QString& id, JsonCallback cb) {
    send_get(QString("/api/v1/instances/%1/console").arg(id), [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        cb(true, QJsonDocument::fromJson(bytes).object(), QString());
    });
}

void AgentClient::set_cracked_mode(const QString& id, bool enabled, JsonCallback cb) {
    QJsonObject body{{"enabled", enabled}};
    send_post(QString("/api/v1/instances/%1/cracked").arg(id), QJsonDocument(body).toJson(QJsonDocument::Compact), [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        cb(true, QJsonDocument::fromJson(bytes).object(), QString());
    });
}

void AgentClient::get_properties(const QString& id, JsonCallback cb) {
    send_get(QString("/api/v1/instances/%1/properties").arg(id), [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        cb(true, QJsonDocument::fromJson(bytes).object(), QString());
    });
}

void AgentClient::update_properties(const QString& id, const QJsonObject& props, JsonCallback cb) {
    send_post(QString("/api/v1/instances/%1/properties").arg(id), QJsonDocument(props).toJson(QJsonDocument::Compact), [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        cb(true, QJsonDocument::fromJson(bytes).object(), QString());
    });
}

void AgentClient::create_backup(const QString& id, JsonCallback cb) {
    send_post(QString("/api/v1/instances/%1/backups").arg(id), "{}", [cb](bool ok, const QByteArray& bytes, const QString& err) {
        if (!ok) { cb(false, {}, err); return; }
        cb(true, QJsonDocument::fromJson(bytes).object(), QString());
    });
}

} // namespace synx::admin

