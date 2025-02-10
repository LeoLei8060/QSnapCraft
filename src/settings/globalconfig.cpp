#include "globalconfig.h"
#include <QDir>
#include <QSettings>

#define INI_PATH "config.ini"

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
    QSettings settings(INI_PATH, QSettings::IniFormat);

    // 加载通用设置
    settings.beginGroup("General");
    m_generalData.savePath = settings.value("SavePath", "config.ini").toString();
    m_generalData.autoStart = settings.value("AutoStart", true).toBool();
    settings.endGroup();

    // 加载控制设置
    settings.beginGroup("Control");
    m_controlData.captureHotkey = settings.value("CaptureHotkey", "F1").toString();
    m_controlData.captureCopyHotkey = settings.value("CaptureCopyHotkey", "Ctrl+F1").toString();
    m_controlData.showLastPinHotkey = settings.value("ShowLastPinHotkey", "F3").toString();
    m_controlData.showPinsHotkey = settings.value("ShowPinsHotkey", "Shift+F3").toString();
    m_controlData.transparencyHotkey = settings.value("TransparencyHotkey", "Ctrl+Wheel").toString();
    m_controlData.closePinHotkey = settings.value("ClosePinHotkey", "DoubleLeftClick").toString();
    settings.endGroup();

    // 加载输出设置
    settings.beginGroup("Output");
    m_outputData.fileNameFormat
        = settings.value("FileNameFormat", "QSnapCraft_$yyyy-MM-dd_HH-mm-ss$.png").toString();
    m_outputData.fastSavePath
        = settings.value("FastSavePath", "QSnapCraft_$yyyy-MM-dd_HH-mm-ss$.png").toString();
    m_outputData.fastSaveHotkey = settings.value("FastSaveHotkey", "Ctrl+Shift+S").toString();
    settings.endGroup();

    // 加载截图设置
    settings.beginGroup("Screenshot");
    m_screenshotData.borderWidth = settings.value("BorderWidth", 1).toUInt();
    m_screenshotData.doubleLeftAction = settings.value("DoubleLeftAction", 0).toInt();
    m_screenshotData.bExit_doubleLeft = settings.value("ExitDoubleLeft", false).toBool();
    m_screenshotData.doubleMiddleAction = settings.value("DoubleMiddleAction", 0).toInt();
    m_screenshotData.bExit_doubleMiddle = settings.value("ExitDoubleMiddle", false).toBool();
    m_screenshotData.enterAction = settings.value("EnterAction", 0).toInt();
    m_screenshotData.bExit_enter = settings.value("ExitEnter", false).toBool();
    settings.endGroup();
}

void GlobalConfig::saveConfig()
{
    QSettings settings(INI_PATH, QSettings::IniFormat);

    // 保存通用设置
    settings.beginGroup("General");
    settings.setValue("SavePath", m_generalData.savePath);
    settings.setValue("AutoStart", m_generalData.autoStart);
    settings.endGroup();

    // 保存控制设置
    settings.beginGroup("Control");
    settings.setValue("CaptureHotkey", m_controlData.captureHotkey);
    settings.setValue("CaptureCopyHotkey", m_controlData.captureCopyHotkey);
    settings.setValue("ShowLastPinHotkey", m_controlData.showLastPinHotkey);
    settings.setValue("ShowPinsHotkey", m_controlData.showPinsHotkey);
    settings.setValue("TransparencyHotkey", m_controlData.transparencyHotkey);
    settings.setValue("ClosePinHotkey", m_controlData.closePinHotkey);
    settings.endGroup();

    // 保存输出设置
    settings.beginGroup("Output");
    settings.setValue("FileNameFormat", m_outputData.fileNameFormat);
    settings.setValue("FastSavePath", m_outputData.fastSavePath);
    settings.setValue("FastSaveHotkey", m_outputData.fastSaveHotkey);
    settings.endGroup();

    // 保存截图设置
    settings.beginGroup("Screenshot");
    settings.setValue("BorderWidth", m_screenshotData.borderWidth);
    settings.setValue("DoubleLeftAction", m_screenshotData.doubleLeftAction);
    settings.setValue("ExitDoubleLeft", m_screenshotData.bExit_doubleLeft);
    settings.setValue("DoubleMiddleAction", m_screenshotData.doubleMiddleAction);
    settings.setValue("ExitDoubleMiddle", m_screenshotData.bExit_doubleMiddle);
    settings.setValue("EnterAction", m_screenshotData.enterAction);
    settings.setValue("ExitEnter", m_screenshotData.bExit_enter);
    settings.endGroup();

    settings.sync();
}
