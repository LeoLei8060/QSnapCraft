#include "globalconfig.h"
#include <QDir>
#include <QSettings>

GlobalConfig *GlobalConfig::m_instance = nullptr;

//GlobalConfig::GeneralSettingsData GlobalConfig::m_generalData;
//GlobalConfig::ControlSettingsData GlobalConfig::m_controlData;
//GlobalConfig::OutputSettingsData GlobalConfig::m_outputData;
//GlobalConfig::ScreenshotSettingsData GlobalConfig::m_screenshotData;

GlobalConfig::GlobalConfig(QObject *parent)
    : QObject(parent)
{
    loadConfig();
}

GlobalConfig::~GlobalConfig()
{
    saveConfig();
}

GlobalConfig *GlobalConfig::instance()
{
    if (!m_instance) {
        m_instance = new GlobalConfig();
    }
    return m_instance;
}

void GlobalConfig::setGeneralData(const GeneralSettingsData &data)
{
    m_generalData = data;
    saveConfig();
    emit configChanged();
}

void GlobalConfig::setControlData(const ControlSettingsData &data)
{
    m_controlData = data;
    saveConfig();
    emit configChanged();
}

void GlobalConfig::setOutputData(const OutputSettingsData &data)
{
    m_outputData = data;
    saveConfig();
    emit configChanged();
}

void GlobalConfig::setScreenshotData(const ScreenshotSettingsData &data)
{
    m_screenshotData = data;
    saveConfig();
    emit configChanged();
}

void GlobalConfig::loadConfig()
{
    QSettings settings;

    // 加载通用设置
    settings.beginGroup("General");
    m_generalData.savePath = settings.value("SavePath", QDir::homePath() + "/Pictures/QSnapCraft")
                                 .toString();
    m_generalData.autoStart = settings.value("AutoStart", false).toBool();
    m_generalData.minimizeToTray = settings.value("MinimizeToTray", true).toBool();
    m_generalData.closeToTray = settings.value("CloseToTray", true).toBool();
    m_generalData.language = settings.value("Language", "zh_CN").toString();
    settings.endGroup();

    // 加载控制设置
    settings.beginGroup("Control");
    m_controlData.captureHotkey = settings.value("CaptureHotkey", "Ctrl+Alt+A").toString();
    m_controlData.pinHotkey = settings.value("PinHotkey", "Ctrl+Alt+P").toString();
    m_controlData.recordHotkey = settings.value("RecordHotkey", "Ctrl+Alt+R").toString();
    m_controlData.enableSoundEffect = settings.value("EnableSoundEffect", true).toBool();
    m_controlData.mouseSpeed = settings.value("MouseSpeed", 5).toInt();
    settings.endGroup();

    // 加载输出设置
    settings.beginGroup("Output");
    m_outputData.imageFormat = settings.value("ImageFormat", "PNG").toString();
    m_outputData.imageQuality = settings.value("ImageQuality", 90).toInt();
    m_outputData.videoFormat = settings.value("VideoFormat", "MP4").toString();
    m_outputData.videoQuality = settings.value("VideoQuality", 75).toInt();
    m_outputData.preserveAspectRatio = settings.value("PreserveAspectRatio", true).toBool();
    settings.endGroup();

    // 加载截图设置
    settings.beginGroup("Screenshot");
    m_screenshotData.showMagnifier = settings.value("ShowMagnifier", true).toBool();
    m_screenshotData.showSize = settings.value("ShowSize", true).toBool();
    m_screenshotData.showMouseCursor = settings.value("ShowMouseCursor", true).toBool();
    m_screenshotData.magnifierSize = settings.value("MagnifierSize", 150).toInt();
    m_screenshotData.includeWindowShadow = settings.value("IncludeWindowShadow", true).toBool();
    settings.endGroup();
}

void GlobalConfig::saveConfig()
{
    QSettings settings;

    // 保存通用设置
    settings.beginGroup("General");
    settings.setValue("SavePath", m_generalData.savePath);
    settings.setValue("AutoStart", m_generalData.autoStart);
    settings.setValue("MinimizeToTray", m_generalData.minimizeToTray);
    settings.setValue("CloseToTray", m_generalData.closeToTray);
    settings.setValue("Language", m_generalData.language);
    settings.endGroup();

    // 保存控制设置
    settings.beginGroup("Control");
    settings.setValue("CaptureHotkey", m_controlData.captureHotkey);
    settings.setValue("PinHotkey", m_controlData.pinHotkey);
    settings.setValue("RecordHotkey", m_controlData.recordHotkey);
    settings.setValue("EnableSoundEffect", m_controlData.enableSoundEffect);
    settings.setValue("MouseSpeed", m_controlData.mouseSpeed);
    settings.endGroup();

    // 保存输出设置
    settings.beginGroup("Output");
    settings.setValue("ImageFormat", m_outputData.imageFormat);
    settings.setValue("ImageQuality", m_outputData.imageQuality);
    settings.setValue("VideoFormat", m_outputData.videoFormat);
    settings.setValue("VideoQuality", m_outputData.videoQuality);
    settings.setValue("PreserveAspectRatio", m_outputData.preserveAspectRatio);
    settings.endGroup();

    // 保存截图设置
    settings.beginGroup("Screenshot");
    settings.setValue("ShowMagnifier", m_screenshotData.showMagnifier);
    settings.setValue("ShowSize", m_screenshotData.showSize);
    settings.setValue("ShowMouseCursor", m_screenshotData.showMouseCursor);
    settings.setValue("MagnifierSize", m_screenshotData.magnifierSize);
    settings.setValue("IncludeWindowShadow", m_screenshotData.includeWindowShadow);
    settings.endGroup();

    settings.sync();
}
