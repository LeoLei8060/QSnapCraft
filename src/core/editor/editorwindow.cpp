#include "editorwindow.h"
#include "colorpick.h"
#include "shapes/arrow.h"
#include "shapes/ellipse.h"
#include "shapes/eraser.h"
#include "shapes/freehand.h"
#include "shapes/highlighter.h"
#include "shapes/mosaic.h"
#include "shapes/polyline.h"
#include "shapes/rectangle.h"
#include "shapes/shape.h"
#include "shapes/text.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QStandardPaths>

// 初始化静态成员
QColor EditorWindow::s_lastUsedColor = Qt::black;

EditorWindow::EditorWindow(QWidget *parent)
    : QWidget(parent)
    , m_toolbar(this)
    , m_currentMode(DrawMode::Move)
    , m_mosaicType(0)
    , m_penColor(s_lastUsedColor) // 使用上次使用的颜色
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Widget);
    //    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_StaticContents);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    setFocusPolicy(Qt::StrongFocus);         // 允许接收键盘焦点
    setAttribute(Qt::WA_InputMethodEnabled); // 启用输入法
    setMouseTracking(true);

    // 连接信号
    connect(&m_toolbar, &Toolbar::toolSelected, this, &EditorWindow::onToolSelected);
    m_toolbar.hide(); // 初始时隐藏工具栏

    const auto screens = QGuiApplication::screens();
    for (const auto *screen : screens) {
        m_screenGeometries[const_cast<QScreen *>(screen)] = screen->geometry();
        m_totalGeometry = m_totalGeometry.united(screen->geometry());
    }
    setGeometry(m_totalGeometry);

    for (auto *screen : screens) {
        connect(screen, &QScreen::geometryChanged, this, [this, screen](const QRect &geometry) {
            m_screenGeometries[screen] = geometry;
            m_totalGeometry = QRect();
            for (const auto &rect : m_screenGeometries) {
                m_totalGeometry = m_totalGeometry.united(rect);
            }
            setGeometry(m_totalGeometry);
        });
    }

    setMouseTracking(true);
}

void EditorWindow::start(const QPixmap &image, const QRect &captureRect)
{
    m_dragStartPos = QPoint();
    m_screenshotPixmap = image;
    m_captureRect = captureRect;
    m_toolbar.show();
    updateToolbarPosition();
    show();
}

void EditorWindow::hideWindow()
{
    m_toolbar.hide();
    hide();
}

void EditorWindow::setDrawMode(int mode)
{
    m_currentMode = static_cast<DrawMode>(mode);
    if (m_currentMode == DrawMode::DrawTexts) {
        setCursor(Qt::IBeamCursor);
    } else {
        setCursor(Qt::ArrowCursor);
        clearFocus(); // 切换到其他模式时清除焦点
    }

    if (m_currentShape) {
        if (m_currentMode == DrawMode::DrawTexts && dynamic_cast<Text *>(m_currentShape.get())) {
            // 如果当前正在编辑文本，不要重置
            return;
        }
        finishCurrentShape();
        update();
    }
}

void EditorWindow::setPenColor(const QColor &color)
{
    m_penColor = color;
    s_lastUsedColor = color; // 保存颜色
    if (m_currentShape) {
        m_currentShape->setPenColor(color);
        update();
    }
    updateColorButton();
}

void EditorWindow::updateColorButton()
{
    if (auto *colorBtn = m_toolbar.getColorButton()) {
        QString style = QString(R"(
            QToolButton {
                background-color: %1;
                border: 1px solid transparent;
                border-radius: 4px;
                margin: 8px;
            }
            QToolButton:hover {
                border: 1px solid #6a6a6a;
            }
            QToolButton:pressed {
                border: 1px solid #6a6a6a;
                background-color: rgba(%2, %3, %4, 0.8);
            }
        )")
                            .arg(m_penColor.name())
                            .arg(m_penColor.red())
                            .arg(m_penColor.green())
                            .arg(m_penColor.blue());
        colorBtn->setStyleSheet(style);
    }
}

void EditorWindow::undo()
{
    if (!m_shapes.empty()) {
        m_redoStack.push_back(std::move(m_shapes.back()));
        m_shapes.pop_back();
        update();
    }
}

void EditorWindow::redo()
{
    if (!m_redoStack.empty()) {
        m_shapes.push_back(std::move(m_redoStack.back()));
        m_redoStack.pop_back();
        update();
    }
}

void EditorWindow::updateToolbarPosition()
{
    const int toolbarWidth = m_toolbar.width();
    const int toolbarHeight = m_toolbar.height();
    const int spacing = 8;

    int x = m_captureRect.right() - toolbarWidth;
    int y = m_captureRect.bottom() + spacing;

    if (x + toolbarWidth > m_totalGeometry.right()) {
        x = m_totalGeometry.right() - toolbarWidth;
    }
    if (x < m_totalGeometry.left()) {
        x = m_totalGeometry.left();
    }

    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen->geometry().contains(x, 1)) {
            if (y + toolbarHeight > screen->geometry().bottom()) {
                y = m_captureRect.top() - toolbarHeight - spacing;
            }
            break;
        }
    }

    m_toolbar.move(x, y);
    m_toolbar.raise();
    m_toolbar.activateWindow(); // 激活工具栏窗口
}

void EditorWindow::onToolSelected(Toolbar::Tool tool)
{
    switch (tool) {
    case Toolbar::Tool::MoveBtn:
        setDrawMode(DrawMode::Move);
        break;
    case Toolbar::Tool::RectangleBtn:
        setDrawMode(DrawMode::DrawRectangle);
        break;
    case Toolbar::Tool::EllipseBtn:
        setDrawMode(DrawMode::DrawEllipse);
        break;
    case Toolbar::Tool::PolyLineBtn:
        setDrawMode(DrawMode::DrawPolyline);
        break;
    case Toolbar::Tool::ArrowBtn:
        setDrawMode(DrawMode::DrawArrow);
        break;
    case Toolbar::Tool::PencilBtn:
        setDrawMode(DrawMode::DrawFreehand);
        break;
    case Toolbar::Tool::MarkerBtn:
        setDrawMode(DrawMode::DrawHighlighter);
        break;
    case Toolbar::Tool::MosaicBtn:
        setDrawMode(DrawMode::DrawMosaic);
        break;
    case Toolbar::Tool::TextBtn:
        setDrawMode(DrawMode::DrawTexts);
        break;
    case Toolbar::Tool::Eraser:
        setDrawMode(DrawMode::DrawEraser);
        break;
    case Toolbar::Tool::Undo:
        undo();
        break;
    case Toolbar::Tool::Redo:
        redo();
        break;
    case Toolbar::Tool::Pin:
        pinImage();
        break;
    case Toolbar::Tool::Exit:
        close();
        break;
    case Toolbar::Tool::Save:
        saveImage();
        break;
    case Toolbar::Tool::Copy:
        copyImage();
        break;
    case Toolbar::Tool::ColorBtn: {
        // 在颜色按钮旁边显示颜色选择器
        if (QToolButton *colorBtn = m_toolbar.getColorButton()) {
            // 创建临时的颜色选择器
            auto *colorPick = new ColorPick(nullptr);
            colorPick->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动删除
            colorPick->setColor(m_penColor);               // 设置当前颜色

            // 连接颜色变化信号
            connect(colorPick, &ColorPick::colorChanged, this, &EditorWindow::setPenColor);

            colorPick->showAtButton(colorBtn);
        }
        break;
    }
    default:
        break;
    }
}

QString EditorWindow::getSaveFilePath()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString defaultName = QString("screenshot_%1.png")
                              .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    return QFileDialog::getSaveFileName(this,
                                        tr("Save Screenshot"),
                                        defaultPath + "/" + defaultName,
                                        tr("Images (*.png *.jpg *.bmp)"));
}

void EditorWindow::editorFinished()
{
    // 先清理当前窗口状态和数据
    setDrawMode(DrawMode::Move);
    m_shapes.clear();
    m_screenshotPixmap = QPixmap();
    m_captureRect = QRect();
    m_dragStartPos = QPoint();

    emit sigEditorFinished();
}

void EditorWindow::saveImage()
{
    QString filePath = getSaveFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    // 创建要保存的图像（仅包含捕获区域）
    auto tempPixmap = m_screenshotPixmap;
    shapesToImage(tempPixmap);
    QPixmap savePixmap = tempPixmap.copy(m_captureRect);

    // 保存图像
    if (!savePixmap.save(filePath)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save the image to %1").arg(filePath));
        return;
    }

    // 截屏结束
    editorFinished();
}

void EditorWindow::copyImage()
{
    // 从截图中裁剪出选择区域
    auto tempPixmap = m_screenshotPixmap;
    shapesToImage(tempPixmap);
    QPixmap capturedPixmap = tempPixmap.copy(m_captureRect);

    // 获取系统剪贴板
    QClipboard *clipboard = QApplication::clipboard();

    // 将图片复制到剪贴板
    clipboard->setPixmap(capturedPixmap);

    editorFinished();
}

void EditorWindow::pinImage()
{
    // 从截图中裁剪出选择区域
    auto tempPixmap = m_screenshotPixmap;
    shapesToImage(tempPixmap);
    QPixmap capturedPixmap = tempPixmap.copy(m_captureRect);

    emit sigPinImage(capturedPixmap, m_captureRect);
    editorFinished();
}

void EditorWindow::shapesToImage(QPixmap &img)
{
    if (m_shapes.size() == 0)
        return;
    auto     tmpImg = img.toImage();
    QPainter painter(&tmpImg);
    for (const auto &shape : m_shapes) {
        shape->draw(painter);
    }

    QPainter imgPainter(&img);
    imgPainter.drawImage(0, 0, tmpImg);
}

void EditorWindow::createShape(const QPoint &pos)
{
    switch (m_currentMode) {
    case DrawMode::DrawPolyline:
        m_currentShape = std::make_unique<Polyline>(pos, m_penColor);
        break;
    case DrawMode::DrawEllipse:
        m_currentShape = std::make_unique<Ellipse>(pos, m_penColor);
        break;
    case DrawMode::DrawRectangle:
        m_currentShape = std::make_unique<Rectangle>(pos, m_penColor);
        break;
    case DrawMode::DrawArrow:
        m_currentShape = std::make_unique<Arrow>(pos, m_penColor);
        break;
    case DrawMode::DrawFreehand:
        m_currentShape = std::make_unique<Freehand>(pos, m_penColor);
        break;
    case DrawMode::DrawHighlighter:
        m_currentShape = std::make_unique<Highlighter>(pos, m_penColor);
        break;
    case DrawMode::DrawMosaic:
        m_currentShape = std::make_unique<Mosaic>(pos,
                                                  static_cast<Mosaic::MosaicType>(m_mosaicType),
                                                  m_penColor);
        break;
    case DrawMode::DrawEraser:
        m_currentShape = std::make_unique<Eraser>(pos);
        break;
    case DrawMode::DrawTexts:
        m_currentShape = std::make_unique<Text>(pos, m_penColor);
        break;
    default:
        break;
    }
}

void EditorWindow::handleTextInput(const QPoint &pos)
{
    createShape(pos);
}

void EditorWindow::finishCurrentShape()
{
    if (m_currentShape) {
        m_shapes.push_back(std::move(m_currentShape));
        m_redoStack.clear();
    }
}

void EditorWindow::removeCurrentShape()
{
    if (m_currentShape) {
        m_currentShape.reset();
        m_redoStack.clear();
    }
}

void EditorWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(0, 0, m_screenshotPixmap);

    if (m_captureRect.isValid()) {
        QPainterPath windowPath;
        windowPath.addRect(rect());

        QPainterPath highlightPath;
        highlightPath.addRect(m_captureRect);

        QPainterPath overlayPath = windowPath.subtracted(highlightPath);

        painter.fillPath(overlayPath, QColor(0, 0, 0, 128));

        painter.setPen(QPen(Qt::white, 2));
        painter.drawRect(m_captureRect);

        const int handleSize = 8;
        painter.setBrush(Qt::white);
        painter.setPen(Qt::black);

        QVector<QPoint> corners = {m_captureRect.topLeft(),
                                   m_captureRect.topRight(),
                                   m_captureRect.bottomRight(),
                                   m_captureRect.bottomLeft()};

        for (const QPoint &corner : corners) {
            painter.drawRect(QRect(corner.x() - handleSize / 2,
                                   corner.y() - handleSize / 2,
                                   handleSize,
                                   handleSize));
        }

        QPoint midPoints[] = {QPoint(m_captureRect.left(), m_captureRect.center().y()),
                              QPoint(m_captureRect.right(), m_captureRect.center().y()),
                              QPoint(m_captureRect.center().x(), m_captureRect.top()),
                              QPoint(m_captureRect.center().x(), m_captureRect.bottom())};

        for (const QPoint &point : midPoints) {
            painter.drawRect(QRect(point.x() - handleSize / 2,
                                   point.y() - handleSize / 2,
                                   handleSize,
                                   handleSize));
        }
    }

    auto     paintImg = QImage(size(), QImage::Format_ARGB32);
    QPainter imgPainter(&paintImg);

    // Draw all completed shapes
    for (const auto &shape : m_shapes) {
        shape->draw(imgPainter);
    }

    // Draw current shape if exists
    if (m_currentShape) {
        m_currentShape->draw(imgPainter);
    }

    // TODO: 暂时先取消编辑窗口放大镜功能
    //    m_magnifier.paint(painter, m_currentImage, QCursor::pos());

    painter.drawImage(0, 0, paintImg);
}

EditorWindow::ResizeHandle EditorWindow::hitTest(const QPoint &pos) const
{
    const int handleSize = 8;

    QRect handleRect;

    handleRect = QRect(m_captureRect.topLeft().x() - handleSize / 2,
                       m_captureRect.topLeft().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::TopLeft;

    handleRect = QRect(m_captureRect.center().x() - handleSize / 2,
                       m_captureRect.top() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Top;

    handleRect = QRect(m_captureRect.topRight().x() - handleSize / 2,
                       m_captureRect.topRight().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::TopRight;

    handleRect = QRect(m_captureRect.right() - handleSize / 2,
                       m_captureRect.center().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Right;

    handleRect = QRect(m_captureRect.bottomRight().x() - handleSize / 2,
                       m_captureRect.bottomRight().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::BottomRight;

    handleRect = QRect(m_captureRect.center().x() - handleSize / 2,
                       m_captureRect.bottom() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Bottom;

    handleRect = QRect(m_captureRect.bottomLeft().x() - handleSize / 2,
                       m_captureRect.bottomLeft().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::BottomLeft;

    handleRect = QRect(m_captureRect.left() - handleSize / 2,
                       m_captureRect.center().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Left;

    if (m_captureRect.contains(pos))
        return ResizeHandle::Move;

    return ResizeHandle::None;
}

void EditorWindow::updateCursor(ResizeHandle handle)
{
    switch (handle) {
    case ResizeHandle::TopLeft:
    case ResizeHandle::BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case ResizeHandle::TopRight:
    case ResizeHandle::BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case ResizeHandle::Left:
    case ResizeHandle::Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case ResizeHandle::Top:
    case ResizeHandle::Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case ResizeHandle::Move:
        setCursor(Qt::SizeAllCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

void EditorWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_currentMode == DrawMode::Move) {
            m_dragStartPos = QCursor::pos();
            m_currentHandle = hitTest(m_dragStartPos);
            m_isDragging = (m_currentHandle != ResizeHandle::None);
            return;
        }
        if (!m_currentShape) {
            if (m_currentMode == DrawMode::DrawTexts) {
                setCursor(Qt::IBeamCursor);
                setFocus(); // 获取键盘焦点

                // 获取点击位置的背景颜色
                QPixmap pixmap = grab(QRect(event->pos(), QSize(1, 1)));
                QImage  image = pixmap.toImage();
                QColor  backgroundColor = image.pixelColor(0, 0);

                createShape(event->pos());
                if (m_currentShape) {
                    auto textShape = static_cast<Text *>(m_currentShape.get());
                    textShape->updateBackgroundColor(backgroundColor);

                    // 获取文本框区域的背景
                    QRect bgRect = textShape->getTextRect();
                    textShape->setBackground(grab(bgRect));
                }
            } else if (m_currentMode == DrawMode::DrawMosaic) {
                m_currentShape = std::make_unique<Mosaic>(event->pos(),
                                                          static_cast<Mosaic::MosaicType>(
                                                              m_mosaicType),
                                                          m_penColor);
                if (auto *mosaic = static_cast<Mosaic *>(m_currentShape.get())) {
                    mosaic->setBackground(grab()); // 获取当前画布内容作为背景
                }
            } else {
                createShape(event->pos());
            }
        } else {
            if (m_currentMode == DrawMode::DrawPolyline) {
                // 对于折线，每次点击都添加新的点
                static_cast<Polyline *>(m_currentShape.get())->addPoint(event->pos());
                // TODO: delete
                //                static_cast<Polyline *>(currentShape.get())->m_points.append(event->pos());
            } else if (m_currentMode == DrawMode::DrawTexts) {
                // 如果点击在文本框外，结束编辑
                auto textShape = static_cast<Text *>(m_currentShape.get());
                if (!textShape->getTextRect().contains(event->pos())) {
                    if (textShape->isEmpty()) {
                        m_currentShape.reset(); // 如果没有输入文本，取消本次操作
                    } else {
                        textShape->finishEdit();
                        finishCurrentShape();
                    }
                    setCursor(Qt::ArrowCursor);
                    clearFocus();
                } else {
                    // 如果点击在文本框内，记录偏移量并更新背景
                    textShape->startMove(event->pos());
                    textShape->setBackground(grab(textShape->getTextRect()));
                }
            } else {
                m_currentShape->updateShape(event->pos());
            }
        }
        update();
    } else if (event->button() == Qt::RightButton) {
        if (m_currentMode == DrawMode::Move) {
            m_toolbar.hide();
            emit sigCancelEditor();
        }
        if (m_currentShape) {
            if (m_currentMode == DrawMode::DrawPolyline) {
                auto tmpObj = static_cast<Polyline *>(m_currentShape.get());
                tmpObj->finishLine();
                if (tmpObj->isInvalid()) {
                    removeCurrentShape();
                    update();
                    return;
                }
            } else if (m_currentMode == DrawMode::DrawMosaic) {
                //                mosaicType = (mosaicType == 0) ? 1 : 0;
                auto tmp = static_cast<Mosaic *>(m_currentShape.get());
                tmp->isComplete();
            }
            finishCurrentShape();
            update();
        }
    }
    //    if (event->button() == Qt::LeftButton) {
    //        m_dragStartPos = QCursor::pos();
    //        m_currentHandle = hitTest(m_dragStartPos);
    //        m_isDragging = (m_currentHandle != ResizeHandle::None);
    //    } else if (event->button() == Qt::RightButton) {
    //        m_toolbar.hide();
    //        emit sigCancelEditor();
    //    }
}

void EditorWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_currentMode == DrawMode::Move) {
        QPoint pos = QCursor::pos();

        if (m_isDragging) {
            adjustRect(pos);
        } else {
            ResizeHandle handle = hitTest(pos);
            updateCursor(handle);
        }
        return;
    }
    if (m_currentShape) {
        if (event->buttons() & Qt::LeftButton || m_currentMode == DrawMode::DrawPolyline) {
            if (m_currentMode == DrawMode::DrawTexts) {
                auto textShape = static_cast<Text *>(m_currentShape.get());
                textShape->updateShape(event->pos());
                // 更新移动后的背景
                textShape->setBackground(grab(textShape->getTextRect()));
            } else {
                m_currentShape->updateShape(event->pos());
            }
            update();
        }
    }
}

void EditorWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_currentMode == DrawMode::Move) {
        if (event->button() == Qt::LeftButton) {
            m_isDragging = false;
            m_currentHandle = ResizeHandle::None;
            updateToolbarPosition();
        }
        return;
    }
    if (event->button() == Qt::LeftButton && m_currentShape) {
        if (m_currentMode != DrawMode::DrawPolyline && m_currentMode != DrawMode::DrawTexts) {
            m_currentShape->updateShape(event->pos());
            // 对于椭圆，检查起点和终点是否重合
            if (m_currentMode == DrawMode::DrawEllipse) {
                auto ellipse = static_cast<Ellipse *>(m_currentShape.get());
                if (ellipse->startPoint() == ellipse->endPoint()) {
                    m_currentShape.reset();
                    return;
                }
            }
            finishCurrentShape();
            update();
        }
    }
}

void EditorWindow::keyPressEvent(QKeyEvent *event)
{
    if (m_currentShape && m_currentMode == DrawMode::DrawTexts) {
        auto textShape = static_cast<Text *>(m_currentShape.get());

        if (event->key() == Qt::Key_Backspace) {
            textShape->removeLastChar();
            update();
        } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (!textShape->isEmpty()) {
                textShape->finishEdit();
                finishCurrentShape();
                setCursor(Qt::ArrowCursor);
                clearFocus();
            }
            update();
        } else {
            QString text = event->text();
            if (!text.isEmpty() && text[0].isPrint()) {
                textShape->appendText(text);
                update();
            }
        }
    }
    QWidget::keyPressEvent(event);
}

void EditorWindow::inputMethodEvent(QInputMethodEvent *event)
{
    if (m_currentShape && m_currentMode == DrawMode::DrawTexts) {
        auto         textShape = static_cast<Text *>(m_currentShape.get());
        QFontMetrics metrics(textShape->getFont());
        int          cursorX = textShape->getTextRect().left() + textShape->getPadding();
        if (!textShape->getText().isEmpty()) {
            cursorX += metrics.horizontalAdvance(textShape->getText());
        }

        switch (event->type()) {
        case QEvent::InputMethod:
            if (!event->commitString().isEmpty()) {
                textShape->appendText(event->commitString());
                update();
            }
            break;
        default:
            break;
        }
    }
    QWidget::inputMethodEvent(event);
}

QVariant EditorWindow::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (m_currentShape && m_currentMode == DrawMode::DrawTexts) {
        auto         textShape = static_cast<Text *>(m_currentShape.get());
        QFontMetrics metrics(textShape->getFont());
        int          cursorX = textShape->getTextRect().left() + textShape->getPadding();
        if (!textShape->getText().isEmpty()) {
            cursorX += metrics.horizontalAdvance(textShape->getText());
        }

        switch (query) {
        case Qt::ImEnabled:
            return true;
        case Qt::ImCursorPosition:
            return textShape->getText().length();
        case Qt::ImAnchorPosition:
            return textShape->getText().length();
        case Qt::ImCursorRectangle:
            return QRect(cursorX,
                         textShape->getTextRect().top(),
                         1,
                         textShape->getTextRect().height());
        case Qt::ImFont:
            return QVariant::fromValue(textShape->getFont());
        case Qt::ImCurrentSelection:
            return QString();
        case Qt::ImMaximumTextLength:
            return 65535;
        case Qt::ImHints:
            return int(Qt::ImhMultiLine | Qt::ImhPreferNumbers | Qt::ImhPreferUppercase
                       | Qt::ImhPreferLowercase | Qt::ImhNoPredictiveText);
        default:
            break;
        }
    }
    return QWidget::inputMethodQuery(query);
}

void EditorWindow::adjustRect(const QPoint &pos)
{
    if (!m_isDragging)
        return;

    QRect  newRect = m_captureRect;
    QPoint delta = pos - m_dragStartPos;

    switch (m_currentHandle) {
    case ResizeHandle::TopLeft:
        newRect.setTopLeft(newRect.topLeft() + delta);
        break;
    case ResizeHandle::TopRight:
        newRect.setTopRight(newRect.topRight() + delta);
        break;
    case ResizeHandle::BottomRight:
        newRect.setBottomRight(newRect.bottomRight() + delta);
        break;
    case ResizeHandle::BottomLeft:
        newRect.setBottomLeft(newRect.bottomLeft() + delta);
        break;
    case ResizeHandle::Left:
        newRect.setLeft(newRect.left() + delta.x());
        break;
    case ResizeHandle::Right:
        newRect.setRight(newRect.right() + delta.x());
        break;
    case ResizeHandle::Top:
        newRect.setTop(newRect.top() + delta.y());
        break;
    case ResizeHandle::Bottom:
        newRect.setBottom(newRect.bottom() + delta.y());
        break;
    case ResizeHandle::Move:
        newRect.translate(delta);
        break;
    default:
        break;
    }

    QScreen *screen = QGuiApplication::screenAt(pos);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (m_currentHandle == ResizeHandle::Move) {
        if (newRect.left() < m_totalGeometry.left())
            newRect.moveLeft(m_totalGeometry.left());
        if (newRect.top() < m_totalGeometry.top())
            newRect.moveTop(m_totalGeometry.top());
        if (newRect.right() > m_totalGeometry.right())
            newRect.moveRight(m_totalGeometry.right());
        if (newRect.bottom() > m_totalGeometry.bottom())
            newRect.moveBottom(m_totalGeometry.bottom());
    } else {
        const QRect &screenRect = m_screenGeometries[screen];

        switch (m_currentHandle) {
        case ResizeHandle::TopLeft:
        case ResizeHandle::Top:
        case ResizeHandle::TopRight:
            if (newRect.top() < screenRect.top())
                newRect.setTop(screenRect.top());
            break;
        case ResizeHandle::BottomLeft:
        case ResizeHandle::Bottom:
        case ResizeHandle::BottomRight:
            if (newRect.bottom() > screenRect.bottom())
                newRect.setBottom(screenRect.bottom());
            break;
        default:
            break;
        }

        switch (m_currentHandle) {
        case ResizeHandle::TopLeft:
        case ResizeHandle::Left:
        case ResizeHandle::BottomLeft:
            if (newRect.left() < screenRect.left())
                newRect.setLeft(screenRect.left());
            break;
        case ResizeHandle::TopRight:
        case ResizeHandle::Right:
        case ResizeHandle::BottomRight:
            if (newRect.right() > screenRect.right())
                newRect.setRight(screenRect.right());
            break;
        default:
            break;
        }
    }

    static const int MIN_SIZE = 10;
    if (newRect.width() < MIN_SIZE) {
        if (m_currentHandle == ResizeHandle::Left || m_currentHandle == ResizeHandle::TopLeft
            || m_currentHandle == ResizeHandle::BottomLeft)
            newRect.setLeft(newRect.right() - MIN_SIZE);
        else
            newRect.setRight(newRect.left() + MIN_SIZE);
    }
    if (newRect.height() < MIN_SIZE) {
        if (m_currentHandle == ResizeHandle::Top || m_currentHandle == ResizeHandle::TopLeft
            || m_currentHandle == ResizeHandle::TopRight)
            newRect.setTop(newRect.bottom() - MIN_SIZE);
        else
            newRect.setBottom(newRect.top() + MIN_SIZE);
    }

    if (newRect != m_captureRect) {
        m_captureRect = newRect.normalized();
        m_dragStartPos = pos;
        updateToolbarPosition();
        update();
    }
}
