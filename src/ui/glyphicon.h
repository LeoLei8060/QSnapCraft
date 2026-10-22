#pragma once

#include "app/applicationaction.h"

#include <QColor>
#include <QIcon>

enum class GlyphIcon {
    kCaptureRegion,
    kActiveWindow,
    kScrollCapture,
    kOcr,
    kColorPicker,
    kScreenRuler,
    kPreferences,
    kSelect,
    kRectangle,
    kEllipse,
    kLine,
    kArrow,
    kPencil,
    kMarker,
    kMosaic,
    kText,
    kEraser,
    kNumber,
    kFill,
    kUndo,
    kRedo,
    kCopy,
    kSave,
    kPin,
    kClear,
    kClose,
    kBold,
    kItalic,
    kTextOutline,
    kTextBackground,
    kRulerOrientation,
    kMeasureRectangle,
    kMeasureDistance,
    kOptions,
    kMinimize,
    kAbout,
    kArrowSolid,
    kArrowSolidTail,
    kArrowHollowTail,
    kMosaicBrush,
    kMosaicArea,
    kBlur
};

QIcon createGlyphIcon(GlyphIcon glyph);
QIcon createGlyphIcon(GlyphIcon glyph, const QColor &normal_color);
QIcon createColorSwatchIcon(const QColor &color);
GlyphIcon applicationActionGlyph(ApplicationAction action);
