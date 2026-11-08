#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include "agent_client.hpp"

namespace synx::admin {

class ServerWizardDialog : public QDialog {
    Q_OBJECT

public:
    explicit ServerWizardDialog(AgentClient* client, QWidget* parent = nullptr);

signals:
    void serverCreated(const QString& id, const QString& name);

private slots:
    void onCreateClicked();

private:
    void setup_ui();

    AgentClient* client_;
    QLineEdit* name_input_;
    QComboBox* type_combo_;
    QLineEdit* version_input_;
    QSpinBox* ram_spin_;
    QSlider* ram_slider_;
    QSpinBox* cpu_spin_;
    QCheckBox* cracked_check_;
    QCheckBox* crossplay_check_;
    QSpinBox* port_spin_;
    QLabel* status_label_;
    QPushButton* create_btn_;
};

} // namespace synx::admin

