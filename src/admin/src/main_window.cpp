#include "main_window.hpp"
#include "server_wizard_dialog.hpp"
#include "admin_theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QDateTime>
#include <QJsonDocument>
#include <QScrollBar>

namespace synx::admin {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      client_(new AgentClient("http://127.0.0.1:8443", "synx_agent_dev_token_default_secret_998244353", this)),
      telemetry_timer_(new QTimer(this)),
      console_timer_(new QTimer(this)) {

    setWindowTitle("SYNX HOSTING ADMIN — Localhost Minecraft Server Management");
    resize(1200, 780);
    setStyleSheet(get_admin_stylesheet());

    setup_ui();

    // Auto-refresh timers
    connect(telemetry_timer_, &QTimer::timeout, this, &MainWindow::refresh_telemetry);
    telemetry_timer_->start(2000); // 2s telemetry

    connect(console_timer_, &QTimer::timeout, this, &MainWindow::refresh_console);
    console_timer_->start(1500); // 1.5s console polling

    // Initial fetch
    refresh_telemetry();
    refresh_instances();
}

void MainWindow::setup_ui() {
    auto* central = new QWidget(this);
    auto* main_layout = new QVBoxLayout(central);
    main_layout->setContentsMargins(16, 16, 16, 16);
    main_layout->setSpacing(14);

    setup_telemetry_bar();
    main_layout->addWidget(node_badge_->parentWidget());

    tabs_ = new QTabWidget(this);
    setup_instances_tab();
    setup_console_tab();
    setup_properties_tab();
    setup_backups_tab();
    setup_network_tab();
    setup_agent_tab();

    main_layout->addWidget(tabs_, 1);
    setCentralWidget(central);
}

void MainWindow::setup_telemetry_bar() {
    auto* bar = new QFrame(this);
    bar->setObjectName("cardFrame");
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(20);

    // Node indicator
    node_badge_ = new QLabel("🟢 NODE 1 — ONLINE (Localhost)", bar);
    node_badge_->setStyleSheet("font-weight: 800; font-size: 13px; color: #10B981;");
    layout->addWidget(node_badge_);

    // CPU Meter
    auto* cpu_box = new QWidget(bar);
    auto* cpu_h = new QHBoxLayout(cpu_box);
    cpu_h->setContentsMargins(0, 0, 0, 0);
    auto* cpu_title = new QLabel("CPU:", cpu_box);
    cpu_title->setStyleSheet("color: #9CA3AF; font-weight: 600;");
    cpu_bar_ = new QProgressBar(cpu_box);
    cpu_bar_->setRange(0, 100);
    cpu_bar_->setValue(0);
    cpu_bar_->setFixedWidth(100);
    cpu_label_ = new QLabel("0%", cpu_box);
    cpu_label_->setFixedWidth(40);
    cpu_h->addWidget(cpu_title);
    cpu_h->addWidget(cpu_bar_);
    cpu_h->addWidget(cpu_label_);
    layout->addWidget(cpu_box);

    // RAM Meter
    auto* ram_box = new QWidget(bar);
    auto* ram_h = new QHBoxLayout(ram_box);
    ram_h->setContentsMargins(0, 0, 0, 0);
    auto* ram_title = new QLabel("RAM:", ram_box);
    ram_title->setStyleSheet("color: #9CA3AF; font-weight: 600;");
    ram_bar_ = new QProgressBar(ram_box);
    ram_bar_->setRange(0, 100);
    ram_bar_->setValue(0);
    ram_bar_->setFixedWidth(100);
    ram_label_ = new QLabel("0 / 0 MB", ram_box);
    ram_h->addWidget(ram_title);
    ram_h->addWidget(ram_bar_);
    ram_h->addWidget(ram_label_);
    layout->addWidget(ram_box);

    // Disk
    disk_label_ = new QLabel("Disk: Free 0 GB", bar);
    disk_label_->setStyleSheet("color: #D1D5DB;");
    layout->addWidget(disk_label_);

    // Instance count
    instances_count_label_ = new QLabel("Active Servers: 0", bar);
    instances_count_label_->setStyleSheet("color: #A78BFA; font-weight: bold;");
    layout->addWidget(instances_count_label_);

    layout->addStretch();

    auto* refresh_btn = new QPushButton("Refresh", bar);
    refresh_btn->setObjectName("btnSecondary");
    connect(refresh_btn, &QPushButton::clicked, this, &MainWindow::refresh_telemetry);
    layout->addWidget(refresh_btn);
}

void MainWindow::setup_instances_tab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    auto* create_btn = new QPushButton("+ New Server", page);
    create_btn->setStyleSheet("font-weight: bold; padding: 7px 16px;");
    connect(create_btn, &QPushButton::clicked, this, &MainWindow::on_action_create);

    btn_start_ = new QPushButton("▶ Start", page);
    btn_start_->setObjectName("btnSuccess");
    connect(btn_start_, &QPushButton::clicked, this, &MainWindow::on_action_start);

    btn_stop_ = new QPushButton("⏹ Stop", page);
    btn_stop_->setObjectName("btnDanger");
    connect(btn_stop_, &QPushButton::clicked, this, &MainWindow::on_action_stop);

    btn_restart_ = new QPushButton("🔄 Restart", page);
    btn_restart_->setObjectName("btnSecondary");
    connect(btn_restart_, &QPushButton::clicked, this, &MainWindow::on_action_restart);

    btn_kill_ = new QPushButton("⚡ Force Kill", page);
    btn_kill_->setObjectName("btnDanger");
    connect(btn_kill_, &QPushButton::clicked, this, &MainWindow::on_action_kill);

    btn_delete_ = new QPushButton("🗑 Delete", page);
    btn_delete_->setObjectName("btnSecondary");
    connect(btn_delete_, &QPushButton::clicked, this, &MainWindow::on_action_delete);

    btn_refresh_ = new QPushButton("Refresh", page);
    btn_refresh_->setObjectName("btnSecondary");
    connect(btn_refresh_, &QPushButton::clicked, this, &MainWindow::refresh_instances);

    toolbar->addWidget(create_btn);
    toolbar->addWidget(btn_start_);
    toolbar->addWidget(btn_stop_);
    toolbar->addWidget(btn_restart_);
    toolbar->addWidget(btn_kill_);
    toolbar->addStretch();
    toolbar->addWidget(btn_delete_);
    toolbar->addWidget(btn_refresh_);
    layout->addLayout(toolbar);

    // Table
    instances_table_ = new QTableWidget(page);
    instances_table_->setColumnCount(8);
    instances_table_->setHorizontalHeaderLabels({"Server ID", "Name", "Type", "Version", "Port", "RAM Limit", "Cracked", "Status"});
    instances_table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    instances_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    instances_table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    instances_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    instances_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    instances_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(instances_table_, &QTableWidget::cellClicked, this, &MainWindow::on_instance_selected);
    layout->addWidget(instances_table_, 1);

    tabs_->addTab(page, "🖥 Server Instances");
}

void MainWindow::setup_console_tab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto* top_bar = new QHBoxLayout();
    auto* label = new QLabel("Active Console:", page);
    label->setStyleSheet("font-weight: bold; color: #9CA3AF;");
    console_instance_combo_ = new QComboBox(page);
    console_instance_combo_->setMinimumWidth(250);
    connect(console_instance_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::refresh_console);

    top_bar->addWidget(label);
    top_bar->addWidget(console_instance_combo_);
    top_bar->addStretch();
    layout->addLayout(top_bar);

    console_log_ = new QPlainTextEdit(page);
    console_log_->setObjectName("consoleOutput");
    console_log_->setReadOnly(true);
    layout->addWidget(console_log_, 1);

    auto* input_bar = new QHBoxLayout();
    console_input_ = new QLineEdit(page);
    console_input_->setPlaceholderText("Enter command (e.g. op player, say Hello, stop, gamemode creative)...");
    connect(console_input_, &QLineEdit::returnPressed, this, &MainWindow::on_send_command);

    console_send_btn_ = new QPushButton("Send", page);
    connect(console_send_btn_, &QPushButton::clicked, this, &MainWindow::on_send_command);

    input_bar->addWidget(console_input_, 1);
    input_bar->addWidget(console_send_btn_);
    layout->addLayout(input_bar);

    tabs_->addTab(page, "⚡ Live Console");
}

void MainWindow::setup_properties_tab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* top_bar = new QHBoxLayout();
    auto* label = new QLabel("Select Server:", page);
    label->setStyleSheet("font-weight: bold; color: #9CA3AF;");
    prop_instance_combo_ = new QComboBox(page);
    prop_instance_combo_->setMinimumWidth(250);
    connect(prop_instance_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        QString id = prop_instance_combo_->currentData().toString();
        if (id.isEmpty()) return;
        client_->get_properties(id, [this](bool ok, const QJsonObject& obj, const QString&) {
            if (!ok) return;
            prop_cracked_chk_->setChecked(obj.value("online-mode").toString() == "false");
            prop_pvp_chk_->setChecked(obj.value("pvp").toString() == "true");
            prop_motd_input_->setText(obj.value("motd").toString());
            prop_difficulty_combo_->setCurrentText(obj.value("difficulty").toString("easy"));
            prop_gamemode_combo_->setCurrentText(obj.value("gamemode").toString("survival"));
            prop_max_players_spin_->setValue(obj.value("max-players").toString("20").toInt());
        });
    });

    top_bar->addWidget(label);
    top_bar->addWidget(prop_instance_combo_);
    top_bar->addStretch();
    layout->addLayout(top_bar);

    auto* box = new QGroupBox("Server Properties (server.properties)", page);
    auto* form = new QFormLayout(box);
    form->setSpacing(10);

    prop_cracked_chk_ = new QCheckBox("Allow Cracked / Offline Players (online-mode=false)", box);
    prop_cracked_chk_->setStyleSheet("color: #A78BFA; font-weight: bold;");
    connect(prop_cracked_chk_, &QCheckBox::toggled, this, &MainWindow::on_toggle_cracked_clicked);
    form->addRow("Offline Players:", prop_cracked_chk_);

    prop_pvp_chk_ = new QCheckBox("Enable Player vs Player (PVP)", box);
    form->addRow("Combat:", prop_pvp_chk_);

    prop_motd_input_ = new QLineEdit("A SYNX HOSTING Minecraft Server", box);
    form->addRow("MOTD Description:", prop_motd_input_);

    prop_difficulty_combo_ = new QComboBox(box);
    prop_difficulty_combo_->addItems({"peaceful", "easy", "normal", "hard"});
    form->addRow("Difficulty:", prop_difficulty_combo_);

    prop_gamemode_combo_ = new QComboBox(box);
    prop_gamemode_combo_->addItems({"survival", "creative", "adventure", "spectator"});
    form->addRow("Default Gamemode:", prop_gamemode_combo_);

    prop_max_players_spin_ = new QSpinBox(box);
    prop_max_players_spin_->setRange(1, 500);
    prop_max_players_spin_->setValue(20);
    form->addRow("Max Players:", prop_max_players_spin_);

    layout->addWidget(box);

    auto* btn_bar = new QHBoxLayout();
    prop_save_btn_ = new QPushButton("Save Properties", page);
    prop_save_btn_->setStyleSheet("font-weight: bold; padding: 8px 20px;");
    connect(prop_save_btn_, &QPushButton::clicked, this, &MainWindow::on_save_properties);
    btn_bar->addStretch();
    btn_bar->addWidget(prop_save_btn_);
    layout->addLayout(btn_bar);

    layout->addStretch();
    tabs_->addTab(page, "⚙ Properties & Config");
}

void MainWindow::setup_backups_tab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* top_bar = new QHBoxLayout();
    auto* label = new QLabel("Target Instance:", page);
    label->setStyleSheet("font-weight: bold; color: #9CA3AF;");
    backup_instance_combo_ = new QComboBox(page);
    backup_instance_combo_->setMinimumWidth(250);

    btn_create_backup_ = new QPushButton("📸 Create Instant Snapshot", page);
    connect(btn_create_backup_, &QPushButton::clicked, this, &MainWindow::on_create_backup);

    top_bar->addWidget(label);
    top_bar->addWidget(backup_instance_combo_);
    top_bar->addWidget(btn_create_backup_);
    top_bar->addStretch();
    layout->addLayout(top_bar);

    backups_table_ = new QTableWidget(page);
    backups_table_->setColumnCount(4);
    backups_table_->setHorizontalHeaderLabels({"Snapshot ID", "Filename", "Size (MB)", "SHA-256 Checksum"});
    backups_table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(backups_table_, 1);

    tabs_->addTab(page, "💾 Backups & Snapshots");
}

void MainWindow::setup_network_tab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* desc = new QLabel(
        "SYNX HOSTING Port Allocator & Relay Gateway\n"
        "Java Edition: TCP 25565 - 25600 | Bedrock Edition (GeyserMC): UDP 19132 - 19150\n"
        "Public Custom Domain Configuration: Setup Cloudflare SRV record _minecraft._tcp.play.yourdomain.com pointing to your WAN IP and allocated port.",
        page
    );
    desc->setStyleSheet("color: #9CA3AF; font-size: 13px; line-height: 1.5;");
    layout->addWidget(desc);

    network_table_ = new QTableWidget(page);
    network_table_->setColumnCount(5);
    network_table_->setHorizontalHeaderLabels({"Port", "Protocol", "Status", "Allocated To", "Connection Address"});
    network_table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(network_table_, 1);

    tabs_->addTab(page, "🌐 Network & Domain");
}

void MainWindow::setup_agent_tab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    auto* box = new QGroupBox("Host Agent Service Status", page);
    auto* form = new QFormLayout(box);
    form->setSpacing(12);

    agent_status_label_ = new QLabel("Connected via HTTP 127.0.0.1:8443 and Named Pipe \\\\.\\pipe\\synx_agent_ipc", box);
    agent_status_label_->setStyleSheet("color: #10B981; font-weight: bold;");
    form->addRow("Connection:", agent_status_label_);

    auto* node_id_label = new QLabel("node-localhost-01", box);
    form->addRow("Node ID:", node_id_label);

    auto* isolation_label = new QLabel("Windows Job Objects (Kernel-enforced hard memory & CPU caps)", box);
    form->addRow("Isolation Engine:", isolation_label);

    auto* crk_label = new QLabel("First-class support enabled (online-mode=false toggleable per-server)", box);
    crk_label->setStyleSheet("color: #A78BFA; font-weight: bold;");
    form->addRow("Cracked Players:", crk_label);

    layout->addWidget(box);
    layout->addStretch();

    tabs_->addTab(page, "⚙ Host Agent Settings");
}

void MainWindow::refresh_telemetry() {
    client_->get_metrics([this](bool ok, const QJsonObject& obj, const QString&) {
        if (!ok) {
            node_badge_->setText("🔴 NODE 1 — AGENT OFFLINE");
            node_badge_->setStyleSheet("font-weight: 800; font-size: 13px; color: #EF4444;");
            return;
        }

        node_badge_->setText("🟢 NODE 1 — ONLINE (Localhost)");
        node_badge_->setStyleSheet("font-weight: 800; font-size: 13px; color: #10B981;");

        int cpu = static_cast<int>(obj.value("cpu_usage_percent").toDouble());
        cpu_bar_->setValue(cpu);
        cpu_label_->setText(QString("%1%").arg(cpu));

        double ram_used = obj.value("ram_used_bytes").toDouble() / (1024.0 * 1024.0);
        double ram_total = obj.value("ram_total_bytes").toDouble() / (1024.0 * 1024.0);
        int ram_pct = ram_total > 0 ? static_cast<int>((ram_used / ram_total) * 100.0) : 0;
        ram_bar_->setValue(ram_pct);
        ram_label_->setText(QString("%1 / %2 MB").arg(static_cast<int>(ram_used)).arg(static_cast<int>(ram_total)));

        double disk_free = obj.value("disk_free_bytes").toDouble() / (1024.0 * 1024.0 * 1024.0);
        disk_label_->setText(QString("Disk: Free %1 GB").arg(QString::number(disk_free, 'f', 1)));

        int count = obj.value("total_instances_count").toInt();
        instances_count_label_->setText(QString("Active Servers: %1").arg(count));
    });
}

void MainWindow::refresh_instances() {
    client_->list_instances([this](bool ok, const QJsonArray& arr, const QString&) {
        if (!ok) return;

        instances_table_->setRowCount(arr.size());
        console_instance_combo_->clear();
        prop_instance_combo_->clear();
        backup_instance_combo_->clear();

        for (int i = 0; i < arr.size(); ++i) {
            auto obj = arr[i].toObject();
            QString id = obj.value("id").toString();
            QString name = obj.value("name").toString();
            QString type = obj.value("server_type").toString();
            QString ver = obj.value("mc_version").toString();
            int port = obj.value("port").toInt();
            int ram = obj.value("ram_limit_mb").toInt();
            bool cracked = obj.value("cracked_enabled").toBool();
            QString state = obj.value("state").toString();

            instances_table_->setItem(i, 0, new QTableWidgetItem(id));
            instances_table_->setItem(i, 1, new QTableWidgetItem(name));
            instances_table_->setItem(i, 2, new QTableWidgetItem(type));
            instances_table_->setItem(i, 3, new QTableWidgetItem(ver));
            instances_table_->setItem(i, 4, new QTableWidgetItem(QString::number(port)));
            instances_table_->setItem(i, 5, new QTableWidgetItem(QString("%1 MB").arg(ram)));

            auto* cracked_item = new QTableWidgetItem(cracked ? "Yes (Offline)" : "No (Premium)");
            if (cracked) cracked_item->setForeground(QBrush(QColor("#A78BFA")));
            instances_table_->setItem(i, 6, cracked_item);

            auto* state_item = new QTableWidgetItem(state.toUpper());
            if (state == "running") state_item->setForeground(QBrush(QColor("#10B981")));
            else if (state == "starting") state_item->setForeground(QBrush(QColor("#F59E0B")));
            else state_item->setForeground(QBrush(QColor("#6B7280")));
            instances_table_->setItem(i, 7, state_item);

            QString combo_label = QString("%1 (%2)").arg(name).arg(id.left(8));
            console_instance_combo_->addItem(combo_label, id);
            prop_instance_combo_->addItem(combo_label, id);
            backup_instance_combo_->addItem(combo_label, id);
        }
    });
}

void MainWindow::refresh_console() {
    if (console_instance_combo_->count() == 0) return;
    QString id = console_instance_combo_->currentData().toString();
    if (id.isEmpty()) return;

    client_->get_console_lines(id, [this](bool ok, const QJsonObject& obj, const QString&) {
        if (!ok) return;
        auto lines = obj.value("lines").toArray();
        QStringList full_text;
        for (const auto& line_val : lines) {
            full_text << line_val.toString();
        }
        console_log_->setPlainText(full_text.join("\n"));
        console_log_->verticalScrollBar()->setValue(console_log_->verticalScrollBar()->maximum());
    });
}

QString MainWindow::get_selected_instance_id() const {
    int row = instances_table_->currentRow();
    if (row < 0 || row >= instances_table_->rowCount()) return QString();
    auto* item = instances_table_->item(row, 0);
    return item ? item->text() : QString();
}

void MainWindow::on_instance_selected(int, int) {}

void MainWindow::on_action_start() {
    QString id = get_selected_instance_id();
    if (id.isEmpty()) return;
    client_->send_power_action(id, "start", [this](bool, const QJsonObject&, const QString&) {
        refresh_instances();
    });
}

void MainWindow::on_action_stop() {
    QString id = get_selected_instance_id();
    if (id.isEmpty()) return;
    client_->send_power_action(id, "stop", [this](bool, const QJsonObject&, const QString&) {
        refresh_instances();
    });
}

void MainWindow::on_action_restart() {
    QString id = get_selected_instance_id();
    if (id.isEmpty()) return;
    client_->send_power_action(id, "restart", [this](bool, const QJsonObject&, const QString&) {
        refresh_instances();
    });
}

void MainWindow::on_action_kill() {
    QString id = get_selected_instance_id();
    if (id.isEmpty()) return;
    client_->send_power_action(id, "kill", [this](bool, const QJsonObject&, const QString&) {
        refresh_instances();
    });
}

void MainWindow::on_action_delete() {
    QString id = get_selected_instance_id();
    if (id.isEmpty()) return;
    auto ans = QMessageBox::question(this, "Confirm Deletion", QString("Permanently delete server instance %1?").arg(id));
    if (ans == QMessageBox::Yes) {
        client_->delete_instance(id, [this](bool, const QJsonObject&, const QString&) {
            refresh_instances();
        });
    }
}

void MainWindow::on_action_create() {
    auto* dlg = new ServerWizardDialog(client_, this);
    connect(dlg, &ServerWizardDialog::serverCreated, this, [this](const QString&, const QString&) {
        refresh_instances();
    });
    dlg->exec();
}

void MainWindow::on_send_command() {
    QString id = console_instance_combo_->currentData().toString();
    QString cmd = console_input_->text().trimmed();
    if (id.isEmpty() || cmd.isEmpty()) return;

    console_input_->clear();
    client_->send_command(id, cmd, [this](bool, const QJsonObject&, const QString&) {
        refresh_console();
    });
}

void MainWindow::on_save_properties() {
    QString id = prop_instance_combo_->currentData().toString();
    if (id.isEmpty()) return;

    QJsonObject updates;
    updates["online-mode"] = prop_cracked_chk_->isChecked() ? "false" : "true";
    updates["pvp"] = prop_pvp_chk_->isChecked() ? "true" : "false";
    updates["motd"] = prop_motd_input_->text();
    updates["difficulty"] = prop_difficulty_combo_->currentText();
    updates["gamemode"] = prop_gamemode_combo_->currentText();
    updates["max-players"] = QString::number(prop_max_players_spin_->value());

    client_->update_properties(id, updates, [this](bool ok, const QJsonObject&, const QString& err) {
        if (ok) {
            QMessageBox::information(this, "Success", "Server properties updated successfully.");
            refresh_instances();
        } else {
            QMessageBox::critical(this, "Error", QString("Failed to save properties: %1").arg(err));
        }
    });
}

void MainWindow::on_toggle_cracked_clicked(bool checked) {
    QString id = prop_instance_combo_->currentData().toString();
    if (id.isEmpty()) return;
    client_->set_cracked_mode(id, checked, [this](bool, const QJsonObject&, const QString&) {
        refresh_instances();
    });
}

void MainWindow::on_create_backup() {
    QString id = backup_instance_combo_->currentData().toString();
    if (id.isEmpty()) return;

    btn_create_backup_->setEnabled(false);
    btn_create_backup_->setText("Creating snapshot...");

    client_->create_backup(id, [this](bool ok, const QJsonObject& obj, const QString& err) {
        btn_create_backup_->setEnabled(true);
        btn_create_backup_->setText("📸 Create Instant Snapshot");

        if (ok) {
            int row = backups_table_->rowCount();
            backups_table_->insertRow(row);
            backups_table_->setItem(row, 0, new QTableWidgetItem(obj.value("id").toString()));
            backups_table_->setItem(row, 1, new QTableWidgetItem(obj.value("filename").toString()));
            double mb = obj.value("size_bytes").toDouble() / (1024.0 * 1024.0);
            backups_table_->setItem(row, 2, new QTableWidgetItem(QString::number(mb, 'f', 2)));
            backups_table_->setItem(row, 3, new QTableWidgetItem(obj.value("sha256").toString().left(16) + "..."));
            QMessageBox::information(this, "Snapshot Created", "Verified SHA-256 backup archive created.");
        } else {
            QMessageBox::critical(this, "Backup Failed", err);
        }
    });
}

} // namespace synx::admin
