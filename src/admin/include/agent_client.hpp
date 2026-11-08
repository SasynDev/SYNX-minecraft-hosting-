#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>

namespace synx::admin {

class AgentClient : public QObject {
    Q_OBJECT

public:
    explicit AgentClient(
        const QString& base_url = "http://127.0.0.1:8443",
        const QString& auth_token = "synx_agent_dev_token_default_secret_998244353",
        QObject* parent = nullptr
    );

    using JsonCallback = std::function<void(bool success, const QJsonObject& data, const QString& error)>;
    using ArrayCallback = std::function<void(bool success, const QJsonArray& data, const QString& error)>;

    void get_health(JsonCallback cb);
    void get_metrics(JsonCallback cb);
    void list_instances(ArrayCallback cb);
    void get_instance(const QString& id, JsonCallback cb);
    void create_instance(const QJsonObject& payload, JsonCallback cb);
    void delete_instance(const QString& id, JsonCallback cb);
    void send_power_action(const QString& id, const QString& action, JsonCallback cb);
    void send_command(const QString& id, const QString& command, JsonCallback cb);
    void get_console_lines(const QString& id, JsonCallback cb);
    void set_cracked_mode(const QString& id, bool enabled, JsonCallback cb);
    void get_properties(const QString& id, JsonCallback cb);
    void update_properties(const QString& id, const QJsonObject& props, JsonCallback cb);
    void create_backup(const QString& id, JsonCallback cb);

private:
    QNetworkRequest create_request(const QString& endpoint) const;
    void send_get(const QString& endpoint, std::function<void(bool, const QByteArray&, const QString&)> cb);
    void send_post(const QString& endpoint, const QByteArray& data, std::function<void(bool, const QByteArray&, const QString&)> cb);
    void send_delete(const QString& endpoint, std::function<void(bool, const QByteArray&, const QString&)> cb);

    QString base_url_;
    QString auth_token_;
    QNetworkAccessManager* nam_;
};

} // namespace synx::admin

