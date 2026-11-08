#pragma once

#include <QString>

namespace synx::admin {

inline QString get_admin_stylesheet() {
    return QString(R"(
        QWidget {
            background-color: #0B0D17;
            color: #F3F4F6;
            font-family: 'Segoe UI', 'Plus Jakarta Sans', system-ui, sans-serif;
            font-size: 13px;
        }

        QMainWindow, QDialog {
            background-color: #0B0D17;
        }

        /* Top Header & Cards */
        QFrame#cardFrame, QGroupBox {
            background-color: #151828;
            border: 1px solid #232742;
            border-radius: 8px;
            margin-top: 6px;
            padding: 10px;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 6px;
            color: #A78BFA;
            font-weight: bold;
        }

        /* Tables & Lists */
        QTableWidget, QTreeWidget, QListWidget {
            background-color: #111422;
            border: 1px solid #232742;
            border-radius: 6px;
            gridline-color: #1F2438;
            selection-background-color: #7C3AED;
            selection-color: #FFFFFF;
            alternate-background-color: #151828;
        }

        QHeaderView::section {
            background-color: #1A1E32;
            color: #9CA3AF;
            font-weight: bold;
            padding: 6px 10px;
            border: 1px solid #232742;
        }

        /* Buttons */
        QPushButton {
            background-color: #7C3AED;
            color: #FFFFFF;
            border: none;
            border-radius: 6px;
            padding: 6px 14px;
            font-weight: 600;
        }

        QPushButton:hover {
            background-color: #6D28D9;
        }

        QPushButton:pressed {
            background-color: #5B21B6;
        }

        QPushButton:disabled {
            background-color: #232742;
            color: #6B7280;
        }

        QPushButton#btnSuccess {
            background-color: #10B981;
        }
        QPushButton#btnSuccess:hover {
            background-color: #059669;
        }

        QPushButton#btnDanger {
            background-color: #EF4444;
        }
        QPushButton#btnDanger:hover {
            background-color: #DC2626;
        }

        QPushButton#btnSecondary {
            background-color: #1F2438;
            color: #D1D5DB;
            border: 1px solid #2D3352;
        }
        QPushButton#btnSecondary:hover {
            background-color: #2A304D;
        }

        /* Inputs */
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QComboBox {
            background-color: #0F121F;
            border: 1px solid #282D4A;
            border-radius: 6px;
            color: #F3F4F6;
            padding: 5px 8px;
        }

        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus {
            border: 1px solid #7C3AED;
        }

        QComboBox QAbstractItemView {
            background-color: #151828;
            border: 1px solid #2D3352;
            selection-background-color: #7C3AED;
            color: #FFFFFF;
        }

        /* Terminal Console */
        QPlainTextEdit#consoleOutput {
            background-color: #05070D;
            border: 1px solid #1F2438;
            border-radius: 6px;
            color: #10B981;
            font-family: 'JetBrains Mono', 'Consolas', 'Courier New', monospace;
            font-size: 12px;
            line-height: 1.4;
        }

        /* Scrollbars */
        QScrollBar:vertical {
            background-color: #0B0D17;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background-color: #232742;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #7C3AED;
        }

        /* Tab Widget */
        QTabWidget::pane {
            border: 1px solid #232742;
            background-color: #111422;
            border-radius: 6px;
        }
        QTabBar::tab {
            background-color: #151828;
            color: #9CA3AF;
            border: 1px solid #232742;
            border-bottom: none;
            padding: 8px 18px;
            margin-right: 2px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            font-weight: 600;
        }
        QTabBar::tab:selected {
            background-color: #7C3AED;
            color: #FFFFFF;
        }
        QTabBar::tab:hover:!selected {
            background-color: #1E233A;
            color: #E5E7EB;
        }

        /* CheckBox */
        QCheckBox {
            spacing: 8px;
            color: #F3F4F6;
            font-weight: 500;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #3F4668;
            border-radius: 4px;
            background-color: #0F121F;
        }
        QCheckBox::indicator:checked {
            background-color: #7C3AED;
            border: 1px solid #7C3AED;
        }

        /* Progress Bar */
        QProgressBar {
            background-color: #151828;
            border: 1px solid #232742;
            border-radius: 4px;
            text-align: center;
            color: #FFFFFF;
            font-size: 11px;
            font-weight: bold;
        }
        QProgressBar::chunk {
            background-color: #7C3AED;
            border-radius: 3px;
        }
    )");
}

} // namespace synx::admin

