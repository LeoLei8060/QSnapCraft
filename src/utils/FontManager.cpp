#include "FontManager.h"
#include <QFontDatabase>
#include <QDebug>

FontManager* FontManager::s_instance = nullptr;

FontManager* FontManager::instance()
{
    if (!s_instance) {
        s_instance = new FontManager();
    }
    return s_instance;
}

FontManager::FontManager(QObject *parent)
    : QObject(parent)
{
}

bool FontManager::addThirdpartyFont(const QString &path, int type)
{
    // 如果字体已经加载过，直接返回true
    if (m_fonts.contains(type)) {
        return true;
    }

    // 加载字体文件
    int fontId = QFontDatabase::addApplicationFont(path);
    if (fontId == -1) {
        qWarning() << "Failed to load font:" << path;
        return false;
    }

    // 获取字体族名称
    QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) {
        qWarning() << "No font families found in font file:" << path;
        QFontDatabase::removeApplicationFont(fontId);
        return false;
    }

    // 创建字体对象并保存
    QFont font(families.first());
    m_fonts[type] = font;

    qDebug() << "Successfully loaded font:" << path << "with family:" << families.first();
    return true;
}

QFont FontManager::fontAt(int type)
{
    return m_fonts.value(type, QFont());
}
