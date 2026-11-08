#include "server_wizard_dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

namespace synx::admin {

ServerWizardDialog::ServerWizardDialog(AgentClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setWindowTitle("SYNX HOSTING — Create Minecraft Instance");
    resize(520, 560);
    setup_ui();
}

void ServerWizardDialog::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(16);
    main_layout->setContentsMargins(20, 20, 20, 20);

    // Title Header
    auto* header = new QLabel("Create New Minecraft Server Instance", this);
    header->setStyleSheet("font-size: 16px; font-weight: bold; color: #F3F4F6; margin-bottom: 4px;");
    main_layout->addWidget(header);

    auto* subheader = new QLabel("Deploy an isolated bare-metal instance sandboxed via Windows Job Objects.", this);
    subheader->setStyleSheet("color: #9CA3AF; font-size: 12px; margin-bottom: 12px;");
    main_layout->addWidget(subheader);

    // Form Box
    auto* form_box = new QGroupBox("Instance Configuration", this);
    auto* form_layout = new QFormLayout(form_box);
    form_layout->setSpacing(12);

    name_input_ = new QLineEdit(this);
    name_input_->setPlaceholderText("e.g. Localhost-SMP or Survival-Crossplay");
    form_layout->addRow("Server Name:", name_input_);

    type_combo_ = new QComboBox(this);
    type_combo_->addItem("Paper (High Performance)", "paper");
    type_combo_->addItem("Purpur (Maximum Customization)", "purpur");
    type_combo_->addItem("Vanilla (Official Mojang)", "vanilla");
    type_combo_->addItem("Bedrock Dedicated Server", "bedrock");
    type_combo_->addItem("Fabric (Lightweight Modded)", "fabric");
    type_combo_->addItem("Forge (Heavy Modded)", "forge");
    form_layout->addRow("Server Software:", type_combo_);

    version_input_ = new QLineEdit("1.21.4", this);
    form_layout->addRow("Minecraft Version:", version_input_);

    // RAM layout
    auto* ram_widget = new QWidget(this);
    auto* ram_h = new QHBoxLayout(ram_widget);
    ram_h->setContentsMargins(0, 0, 0, 0);
    ram_spin_ = new QSpinBox(this);
    ram_spin_->setRange(512, 32768);
    ram_spin_->setValue(2048);
    ram_spin_->setSuffix(" MB");
    ram_slider_ = new QSlider(Qt::Horizontal, this);
    ram_slider_->setRange(512, 16384);
    ram_slider_->setSingleStep(512);
    ram_slider_->setValue(2048);
    connect(ram_slider_, &QSlider::valueChanged, ram_spin_, &QSpinBox::setValue);
    connect(ram_spin_, &QSpinBox::valueChanged, ram_slider_, &QSlider::setValue);
    ram_h->addWidget(ram_slider_, 1);
    ram_h->addWidget(ram_spin_);
    form_layout->addRow("RAM Limit:", ram_widget);

    cpu_spin_ = new QSpinBox(this);
    cpu_spin_->setRange(10, 100);
    cpu_spin_->setValue(100);
    cpu_spin_->setSuffix(" %");
    form_layout->addRow("CPU Quota:", cpu_spin_);

    port_spin_ = new QSpinBox(this);
    port_spin_->setRange(0, 65535);
    port_spin_->setValue(0);
    port_spin_->setSpecialValueText("Auto-Allocate (25565+)");
    form_layout->addRow("Port Allocation:", port_spin_);

    main_layout->addWidget(form_box);

    // Player Access Box
    auto* access_box = new QGroupBox("Access & Compatibility Modes", this);
    auto* access_layout = new QVBoxLayout(access_box);

    cracked_check_ = new QCheckBox("Allow Cracked / Offline Players (TLauncher, offline accounts)", this);
    cracked_check_->setChecked(true); // Default true per user priority!
    cracked_check_->setStyleSheet("color: #A78BFA; font-weight: bold;");
    auto* cracked_desc = new QLabel("Sets online-mode=false. Players without official Mojang/Microsoft accounts can connect.", this);
    cracked_desc->setStyleSheet("color: #9CA3AF; font-size: 11px; margin-left: 24px;");

    crossplay_check_ = new QCheckBox("Enable Java + Bedrock Crossplay (GeyserMC + Floodgate)", this);
    crossplay_check_->setChecked(true);
    crossplay_check_->setStyleSheet("color: #38BDF8; font-weight: bold;");
    auto* crossplay_desc = new QLabel("Allows mobile, console, and Windows 10/11 Bedrock players to join on UDP 19132.", this);
    crossplay_desc->setStyleSheet("color: #9CA3AF; font-size: 11px; margin-left: 24px;");

    access_layout->addWidget(cracked_check_);
    access_layout->addWidget(cracked_desc);
    access_layout->addSpacing(8);
    access_layout->addWidget(crossplay_check_);
    access_layout->addWidget(crossplay_desc);
    main_layout->addWidget(access_box);

    status_label_ = new QLabel("", this);
    status_label_->setStyleSheet("color: #EF4444; font-size: 12px;");
    main_layout->addWidget(status_label_);

    // Buttons
    auto* btn_layout = new QHBoxLayout();
    auto* cancel_btn = new QPushButton("Cancel", this);
    cancel_btn->setObjectName("btnSecondary");
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);

    create_btn_ = new QPushButton("Create Instance", this);
    connect(create_btn_, &QPushButton::clicked, this, &ServerWizardDialog::onCreateClicked);

    btn_layout->addStretch();
    btn_layout->addWidget(cancel_btn);
    btn_layout->addWidget(create_btn_);
    main_layout->addLayout(btn_layout);
}

void ServerWizardDialog::onCreateClicked() {
    QString name = name_input_->text().trimmed();
    if (name.isEmpty()) {
        status_label_->setText("Error: Server name cannot be empty.");
        return;
    }

    create_btn_->setEnabled(false);
    create_btn_->setText("Creating...");
    status_label_->setStyleSheet("color: #A78BFA;");
    status_label_->setText("Provisioning sandboxed directory and server.properties...");

    QJsonObject payload;
    payload["name"] = name;
    payload["server_type"] = type_combo_->currentData().toString();
    payload["mc_version"] = version_input_->text().trimmed();
    payload["ram_limit_mb"] = ram_spin_->value();
    payload["cpu_limit_pct"] = cpu_spin_->value();
    payload["cracked_enabled"] = cracked_check_->isChecked();
    payload["crossplay_enabled"] = crossplay_check_->isChecked();
    if (port_spin_->value() > 0) {
        payload["port"] = port_spin_->value();
    }

    client_->create_instance(payload, [this, name](bool ok, const QJsonObject& res, const QString& err) {
        create_btn_->setEnabled(true);
        create_btn_->setText("Create Instance");
        if (!ok) {
            status_label_->setStyleSheet("color: #EF4444;");
            status_label_->setText(QString("Creation failed: %1").arg(err));
            return;
        }

        QString id = res.value("id").toString();
        emit serverCreated(id, name);
        accept();
    });
}

} // namespace synx::admin

