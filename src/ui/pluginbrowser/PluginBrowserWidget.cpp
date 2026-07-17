#include "PluginBrowserWidget.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QSettings>
#include <QHeaderView>

PluginBrowserWidget::PluginBrowserWidget(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    // Scan default VST3 path on Windows
    #ifdef _WIN32
    scanPlugins("C:/Program Files/Common Files/VST3");
    #elif __APPLE__
    scanPlugins("/Library/Audio/Plug-Ins/VST3");
    #else
    scanPlugins("/usr/lib/vst3");
    #endif
}

PluginBrowserWidget::~PluginBrowserWidget() {}

void PluginBrowserWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Title
    QLabel* title = new QLabel("Plugin Browser", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 12px; font-weight: bold; padding: 4px; background-color: #1a1a20; border-radius: 4px;");
    mainLayout->addWidget(title);

    // Search and controls
    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(4);

    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Search plugins...");
    m_searchBox->setStyleSheet("padding: 4px; border-radius: 4px; background-color: #0a0a0f; color: #ddd; border: 1px solid #2a2a30;");
    connect(m_searchBox, &QLineEdit::textChanged, this, &PluginBrowserWidget::filterPlugins);
    controlLayout->addWidget(m_searchBox);

    m_refreshButton = new QPushButton("↻", this);
    m_refreshButton->setFixedSize(28, 28);
    m_refreshButton->setToolTip("Rescan plugins");
    m_refreshButton->setStyleSheet("background-color: #2a2a30; color: #ddd; border-radius: 4px; border: 1px solid #3a3a40;");
    connect(m_refreshButton, &QPushButton::clicked, this, [this]() {
        emit scanStarted();
        scanPlugins("C:/Program Files/Common Files/VST3");
    });
    controlLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(controlLayout);

    // Plugin list
    m_pluginList = new QListWidget(this);
    m_pluginList->setStyleSheet(R"(
        QListWidget {
            background-color: #0a0a0f;
            color: #ddd;
            border: 1px solid #1a1a22;
            border-radius: 4px;
            outline: none;
        }
        QListWidget::item {
            padding: 4px 8px;
            border-bottom: 1px solid #1a1a22;
        }
        QListWidget::item:selected {
            background-color: #2a2a3f;
            color: #fff;
        }
        QListWidget::item:hover {
            background-color: #1a1a2a;
        }
    )");
    connect(m_pluginList, &QListWidget::itemClicked, this, &PluginBrowserWidget::onItemClicked);
    connect(m_pluginList, &QListWidget::itemDoubleClicked, this, &PluginBrowserWidget::onItemDoubleClicked);
    mainLayout->addWidget(m_pluginList);

    // Status and actions
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(4);

    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet("color: #888; font-size: 9px;");
    bottomLayout->addWidget(m_statusLabel);

    bottomLayout->addStretch();

    m_addButton = new QPushButton("Add to Track", this);
    m_addButton->setEnabled(false);
    m_addButton->setStyleSheet("background-color: #3a3a4a; color: #ddd; border-radius: 4px; padding: 4px 12px; border: 1px solid #4a4a5a;");
    connect(m_addButton, &QPushButton::clicked, this, [this]() {
        if (hasSelection()) {
            emit pluginAddedToTrack(getSelectedPlugin(), -1);
        }
    });
    bottomLayout->addWidget(m_addButton);

    mainLayout->addLayout(bottomLayout);

    // Initial message
    QListWidgetItem* item = new QListWidgetItem("No plugins found. Click ↻ to scan.", m_pluginList);
    item->setFlags(Qt::NoItemFlags);
    item->setTextAlignment(Qt::AlignCenter);
    item->setForeground(QColor(0x666666));
    m_pluginList->addItem(item);
}

void PluginBrowserWidget::scanPlugins(const QString& path) {
    m_plugins.clear();
    m_filteredPlugins.clear();
    m_pluginList->clear();
    m_statusLabel->setText("Scanning: " + path + "...");

    QDir dir(path);
    if (!dir.exists()) {
        m_statusLabel->setText("Directory does not exist: " + path);
        emit error("Plugin directory not found: " + path);
        return;
    }

    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    int count = 0;

    for (const QFileInfo& file : files) {
        // VST3
        if (file.isDir() && file.suffix().toLower() == "vst3") {
            PluginInfo info;
            info.name = file.baseName();
            info.vendor = "Unknown";
            info.path = file.absoluteFilePath();
            info.format = "VST3";
            info.isSynth = false;
            info.isEffect = true;
            m_plugins.append(info);
            count++;
        }
        // LV2
        else if (file.isDir() && file.fileName().contains(".lv2")) {
            PluginInfo info;
            info.name = file.baseName();
            info.vendor = "Unknown";
            info.path = file.absoluteFilePath();
            info.format = "LV2";
            info.isSynth = false;
            info.isEffect = true;
            m_plugins.append(info);
            count++;
        }
        // CLAP
        else if (file.isFile() && file.suffix().toLower() == "clap") {
            PluginInfo info;
            info.name = file.baseName();
            info.vendor = "Unknown";
            info.path = file.absoluteFilePath();
            info.format = "CLAP";
            info.isSynth = false;
            info.isEffect = true;
            m_plugins.append(info);
            count++;
        }
    }

    m_statusLabel->setText(QString("Found %1 plugins").arg(count));
    emit scanFinished(count);
    populateList();
}

void PluginBrowserWidget::refreshPluginList() {
    populateList();
}

void PluginBrowserWidget::clearPlugins() {
    m_plugins.clear();
    m_filteredPlugins.clear();
    m_pluginList->clear();
    m_statusLabel->setText("Cleared");
    m_addButton->setEnabled(false);
}

void PluginBrowserWidget::filterPlugins(const QString& filter) {
    if (filter.trimmed().isEmpty()) {
        m_filteredPlugins = m_plugins;
    } else {
        m_filteredPlugins.clear();
        for (const PluginInfo& info : m_plugins) {
            if (info.name.contains(filter, Qt::CaseInsensitive) ||
                info.vendor.contains(filter, Qt::CaseInsensitive) ||
                info.format.contains(filter, Qt::CaseInsensitive)) {
                m_filteredPlugins.append(info);
            }
        }
    }
    populateList();
}

void PluginBrowserWidget::populateList() {
    m_pluginList->clear();

    if (m_filteredPlugins.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem("No plugins found", m_pluginList);
        item->setFlags(Qt::NoItemFlags);
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QColor(0x666666));
        m_pluginList->addItem(item);
        return;
    }

    for (const PluginInfo& info : m_filteredPlugins) {
        QString display = QString("%1 [%2] %3")
                              .arg(info.name)
                              .arg(info.format)
                              .arg(info.isSynth ? "🎹" : "🎛️");
        QListWidgetItem* item = new QListWidgetItem(display, m_pluginList);
        item->setData(Qt::UserRole, QVariant::fromValue(info));
        item->setToolTip(QString("Vendor: %1\nPath: %2").arg(info.vendor).arg(info.path));
        m_pluginList->addItem(item);
    }
}

void PluginBrowserWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    if (m_filteredPlugins.isEmpty()) return;

    int index = m_pluginList->row(item);
    if (index >= 0 && index < m_filteredPlugins.size()) {
        m_selectedIndex = index;
        m_addButton->setEnabled(true);
        emit pluginSelected(m_filteredPlugins[index]);
    }
}

void PluginBrowserWidget::onItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    if (m_filteredPlugins.isEmpty()) return;

    int index = m_pluginList->row(item);
    if (index >= 0 && index < m_filteredPlugins.size()) {
        emit pluginDoubleClicked(m_filteredPlugins[index]);
        emit pluginAddedToTrack(m_filteredPlugins[index], -1);
    }
}

PluginInfo PluginBrowserWidget::getSelectedPlugin() const {
    if (hasSelection()) {
        return m_filteredPlugins[m_selectedIndex];
    }
    return PluginInfo();
}