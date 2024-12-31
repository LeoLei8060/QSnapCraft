#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include <QObject>
#include <QMap>
#include <QFont>

class FontManager : public QObject
{
    Q_OBJECT
public:
    // 字体类型枚举
    enum FontType {
        IconFont = 0,  // 图标字体
        // 可以添加更多字体类型...
    };

    static FontManager* instance();

    // 添加第三方字体
    bool addThirdpartyFont(const QString &path, int type);
    
    // 获取指定类型的字体
    QFont fontAt(int type);

private:
    explicit FontManager(QObject *parent = nullptr);
    ~FontManager() override = default;

    // 禁用拷贝和赋值
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    static FontManager* s_instance;
    QMap<int, QFont> m_fonts;  // 字体类型 -> 字体对象映射
};

#endif // FONTMANAGER_H