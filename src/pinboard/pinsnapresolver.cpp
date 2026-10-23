#include "pinsnapresolver.h"

#include <algorithm>
#include <limits>

namespace {
struct AxisCandidate
{
    int position;
    int targetStart;
    int targetEnd;
};

struct AxisSnap
{
    int  offset{0};
    int  position{0};
    int  targetStart{0};
    int  targetEnd{0};
    bool snapped{false};
};

int rightBoundary(const QRect &rect)
{
    return rect.x() + rect.width();
}

int bottomBoundary(const QRect &rect)
{
    return rect.y() + rect.height();
}

AxisSnap findAxisSnap(const QVector<AxisCandidate> &candidates, const QVector<int> &movingPositions, int thresholdPx)
{
    AxisSnap best;
    int      bestDistance = std::numeric_limits<int>::max();
    for (const AxisCandidate &candidate : candidates) {
        for (int movingPosition : movingPositions) {
            const int offset = candidate.position - movingPosition;
            const int distance = qAbs(offset);
            if (distance <= thresholdPx && distance < bestDistance) {
                best.offset = offset;
                best.position = candidate.position;
                best.targetStart = candidate.targetStart;
                best.targetEnd = candidate.targetEnd;
                best.snapped = true;
                bestDistance = distance;
            }
        }
    }
    return best;
}

bool hasPerpendicularProximity(int movingStart, int movingEnd, const AxisCandidate &candidate, int thresholdPx)
{
    return !(movingEnd + thresholdPx < candidate.targetStart || movingStart > candidate.targetEnd + thresholdPx);
}
} // namespace

PinSnapResult PinSnapResolver::resolve(const QRect          &moving,
                                       const QRect          &screen,
                                       const QVector<QRect> &targets,
                                       int                   thresholdPx)
{
    PinSnapResult result{moving, {}};
    if (!moving.isValid() || !screen.isValid() || thresholdPx <= 0) {
        return result;
    }

    QVector<AxisCandidate> xCandidates{{screen.left(), screen.top(), bottomBoundary(screen)},
                                       {rightBoundary(screen), screen.top(), bottomBoundary(screen)}};
    QVector<AxisCandidate> yCandidates{{screen.top(), screen.left(), rightBoundary(screen)},
                                       {bottomBoundary(screen), screen.left(), rightBoundary(screen)}};

    for (const QRect &target : targets) {
        if (!target.isValid()) {
            continue;
        }
        const QVector<AxisCandidate> targetX{{target.left(), target.top(), bottomBoundary(target)},
                                             {rightBoundary(target), target.top(), bottomBoundary(target)}};
        const QVector<AxisCandidate> targetY{{target.top(), target.left(), rightBoundary(target)},
                                             {bottomBoundary(target), target.left(), rightBoundary(target)}};

        for (const AxisCandidate &candidate : targetX) {
            if (hasPerpendicularProximity(moving.top(), bottomBoundary(moving), candidate, thresholdPx)) {
                xCandidates.append(candidate);
            }
        }
        for (const AxisCandidate &candidate : targetY) {
            if (hasPerpendicularProximity(moving.left(), rightBoundary(moving), candidate, thresholdPx)) {
                yCandidates.append(candidate);
            }
        }
    }

    const QVector<int> xPositions{moving.left(), rightBoundary(moving), moving.left() + moving.width() / 2};
    const QVector<int> yPositions{moving.top(), bottomBoundary(moving), moving.top() + moving.height() / 2};
    const AxisSnap     xSnap = findAxisSnap(xCandidates, xPositions, thresholdPx);
    const AxisSnap     ySnap = findAxisSnap(yCandidates, yPositions, thresholdPx);

    if (xSnap.snapped) {
        result.contentRect.translate(xSnap.offset, 0);
        const int guideTop = std::min(result.contentRect.top(), xSnap.targetStart);
        const int guideBottom = std::max(bottomBoundary(result.contentRect), xSnap.targetEnd);
        result.guides.append(QLineF(xSnap.position, guideTop, xSnap.position, guideBottom));
    }
    if (ySnap.snapped) {
        result.contentRect.translate(0, ySnap.offset);
        const int guideLeft = std::min(result.contentRect.left(), ySnap.targetStart);
        const int guideRight = std::max(rightBoundary(result.contentRect), ySnap.targetEnd);
        result.guides.append(QLineF(guideLeft, ySnap.position, guideRight, ySnap.position));
    }
    return result;
}
