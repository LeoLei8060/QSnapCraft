#pragma once

#include "capture/capturetypes.h"
#include "paint/paintscene.h"

#include <QColor>
#include <QWidget>

class QButtonGroup;
class QFontComboBox;
class QHBoxLayout;
class QLabel;
class QSlider;
class QSpinBox;
class QToolButton;
enum class GlyphIcon;

class PaintBar : public QWidget
{
    Q_OBJECT

public:
    explicit PaintBar(QWidget *parent = nullptr);
    ~PaintBar() override;

    void setCurrentTool(PaintTool tool);
    void setCurrentColor(const QColor &color);
    void setCurrentPenWidth(qreal width);
    void setUndoEnabled(bool enabled);
    void setRedoEnabled(bool enabled);

signals:
    void sigToolChanged(PaintTool tool);
    void sigColorChanged(const QColor &color);
    void sigFilledChanged(bool filled);
    void sigPenWidthChanged(qreal width);
    void sigArrowStyleChanged(ArrowStyle style);
    void sigMarkerOpacityChanged(int opacity);
    void sigEffectModeChanged(PaintEffectMode mode);
    void sigEffectLevelChanged(int level);
    void sigFontFamilyChanged(const QString &family);
    void sigFontSizeChanged(int size);
    void sigFontBoldChanged(bool bold);
    void sigFontItalicChanged(bool italic);
    void sigTextOutlinedChanged(bool outlined);
    void sigTextBackgroundChanged(bool enabled);
    void sigCommandRequested(CaptureCommand command);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QToolButton *addCommandButton(GlyphIcon      glyph,
                                  const QString &toolTip,
                                  const QString &objectName,
                                  CaptureCommand command);
    QToolButton *addToolButton(GlyphIcon glyph, const QString &toolTip, PaintTool tool);
    QLabel      *addSecondaryLabel(const QString &text);
    void         addPrimarySeparator(const QString &objectName);
    QWidget     *addSecondarySeparator(const QString &objectName);
    void         updateSecondaryControls();
    void         updateSizeButton();
    void         updatePaletteSelection();
    void         updateEffectModeButton();
    void         updateEffectSlider();
    void         updateEffectLevelToolTip();
    void         setMosaicBrushMode(bool brushMode);
    void         adjustSizeByWheel(int direction);
    void         refreshSize();

    QWidget      *m_primaryRow{nullptr};
    QWidget      *m_secondaryRow{nullptr};
    QWidget      *m_opacityContainer{nullptr};
    QWidget      *m_commonSeparator{nullptr};
    QWidget      *m_colorGroup{nullptr};
    QWidget      *m_paletteContainer{nullptr};
    QWidget      *m_colorSeparator{nullptr};
    QHBoxLayout  *m_primaryLayout{nullptr};
    QHBoxLayout  *m_secondaryLayout{nullptr};
    QToolButton  *m_undoButton{nullptr};
    QToolButton  *m_redoButton{nullptr};
    QToolButton  *m_sizeButton{nullptr};
    QToolButton  *m_fillButton{nullptr};
    QToolButton  *m_colorButton{nullptr};
    QVector<QToolButton *> m_toolButtons;
    QVector<QToolButton *> m_arrowStyleButtons;
    QButtonGroup *m_arrowStyleGroup{nullptr};
    QVector<QToolButton *> m_paletteButtons;
    QButtonGroup *m_mosaicModeGroup{nullptr};
    QToolButton  *m_mosaicBrushButton{nullptr};
    QToolButton  *m_mosaicAreaButton{nullptr};
    QToolButton  *m_effectModeButton{nullptr};
    QSlider      *m_effectLevelSlider{nullptr};
    QFontComboBox *m_fontCombo{nullptr};
    QToolButton  *m_boldButton{nullptr};
    QToolButton  *m_italicButton{nullptr};
    QToolButton  *m_outlineButton{nullptr};
    QToolButton  *m_textBackgroundButton{nullptr};
    QSpinBox     *m_markerOpacitySpin{nullptr};
    QColor        m_color{255, 82, 82};
    PaintTool     m_tool{PaintTool::kNone};
    PaintEffectMode m_effectMode{PaintEffectMode::kMosaic};
    qreal         m_size{2.0};
    int           m_fontSize{24};
    int           m_mosaicLevel{8};
    int           m_blurLevel{8};
    bool          m_mosaicBrushMode{true};
};
