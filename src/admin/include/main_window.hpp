#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QComboBox>
#include <QTimer>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include "agent_client.hpp"

namespace synx::admin {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void refresh_telemetry();
    void refresh_instances();
    void refresh_console();
    void on_instance_selected(int row, int column);
    void on_action_start();
    void on_action_stop();
    void on_action_restart();
    void on_action_kill();
    void on_action_delete();
    void on_action_create();
    void on_send_command();
    void on_save_properties();
    void on_toggle_cracked_clicked(bool checked);
    void on_create_backup();

private:
    void setup_ui();
    void setup_telemetry_bar();
    void setup_instances_tab();
    void setup_console_tab();
    void setup_properties_tab();
    void setup_backups_tab();
    void setup_network_tab();
    void setup_agent_tab();

    QString get_selected_instance_id() const;

    AgentClient* client_;
    QTimer* telemetry_timer_;
    QTimer* console_timer_;

    // Telemetry Widgets
    QLabel* node_badge_;
    QProgressBar* cpu_bar_;
    QLabel* cpu_label_;
    QProgressBar* ram_bar_;
    QLabel* ram_label_;
    QLabel* disk_label_;
    QLabel* instances_count_label_;

    // Tabs
    QTabWidget* tabs_;

    // Tab 1: Instances
    QTableWidget* instances_table_;
    QPushButton* btn_start_;
    QPushButton* btn_stop_;
    QPushButton* btn_restart_;
    QPushButton* btn_kill_;
    QPushButton* btn_delete_;
    QPushButton* btn_refresh_;

    // Tab 2: Console
    QComboBox* console_instance_combo_;
    QPlainTextEdit* console_log_;
    QLineEdit* console_input_;
    QPushButton* console_send_btn_;

    // Tab 3: Properties
    QComboBox* prop_instance_combo_;
    QCheckBox* prop_cracked_chk_;
    QCheckBox* prop_pvp_chk_;
    QLineEdit* prop_motd_input_;
    QComboBox* prop_difficulty_combo_;
    QComboBox* prop_gamemode_combo_;
    QSpinBox* prop_max_players_spin_;
    QPlainTextEdit* prop_raw_editor_;
    QPushButton* prop_save_btn_;

    // Tab 4: Backups
    QComboBox* backup_instance_combo_;
    QPushButton* btn_create_backup_;
    QTableWidget* backups_table_;

    // Tab 5: Network
    QTableWidget* network_table_;

    // Tab 6: Agent Info
    QLabel* agent_status_label_;
};

} // namespace synx::admin
