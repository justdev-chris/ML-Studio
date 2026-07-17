#ifndef PLUGINBROWSERWIDGET_H
#define PLUGINBROWSERWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QMap>

struct PluginInfo {
    QString name;
    QString vendor;
    QString path;
    QString format; // VST3, LV2, CLAP, AU
    QString category;
    bool isSynth;
    bool isEffect;
    QMap<QString, float> parameters;
};

class PluginBrowserWidget : public QWidget {
    Q_OBJECT

public:
    explicit PluginBrowserWidget(QWidget* parent = nullptr);
    ~PluginBrowserWidget();

    void scanPlugins(const QString& path);
    void refreshPluginList();
    void clearPlugins();

    PluginInfo getSelectedPlugin() const;
    bool hasSelection() const { return m_selectedIndex >= 0 && m_selectedIndex < m_plugins.size(); }

signals:
    void pluginSelected(const PluginInfo& info);
    void pluginDoubleClicked(const PluginInfo& info);
    void pluginAddedToTrack(const PluginInfo& info, int trackIndex);
    void scanStarted();
    void scanFinished(int count);
    void error(const QString& message);

public slots:
    void filterPlugins(const QString& filter);
    void onItemClicked(QListWidgetItem* item);
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    void setupUI();
    void populateList();

    QLineEdit* m_searchBox;
    QListWidget* m_pluginList;
    QLabel* m_statusLabel;
    QPushButton* m_refreshButton;
    QPushButton* m_addButton;

    QVector<PluginInfo> m_plugins;
    QVector<PluginInfo> m_filteredPlugins;
    int m_selectedIndex = -1;

    QStringList m_categories = {
        "All",
        "Synth",
        "Sampler",
        "EQ",
        "Compressor",
        "Reverb",
        "Delay",
        "Filter",
        "Distortion",
        "Modulation",
        "Utility",
        "Other"
    };
};

#endif // PLUGINBROWSERWIDGET_H