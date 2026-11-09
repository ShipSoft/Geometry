// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

// Lays out rendered geometry figures as a multi-page PDF.
//
// Usage:
//   make_figure_pdf <spec.json>
//
// make_visualisations.py writes the spec: the output file, and for each page its
// image, text, legend, axes and labels. Qt's offscreen platform means that no
// display is needed.

#include <QCoreApplication>
#include <QFile>
#include <QFont>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPainterPath>
#include <QPdfWriter>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// Page regions, as fractions of the page. The left column is the axes', so that
// they can never land on the detector.
constexpr double kImageLeft = 0.13;
constexpr double kImageRight = 0.97;
constexpr double kImageTop = 0.14;
constexpr double kImageBottom = 0.84;
constexpr double kAxesLeft = 0.015;
constexpr double kAxesWidth = 0.10;
constexpr double kAxesCentreY = 0.76;
constexpr double kLegendY = 0.925;

// Anything darker than this in any channel counts as drawn, not as background.
constexpr int kBackgroundLevel = 250;
constexpr int kCropMarginPx = 12;

constexpr int kResolutionDpi = 300;

/**
 * @brief An image cropped to what is drawn, remembering where the crop came from.
 */
struct Cropped {
    QImage image;
    QPointF origin;  //!< Top left of the crop, in pixels of the uncropped image.
    QPointF centre;  //!< Centre of the uncropped image, which is the camera target.
};

/**
 * @brief Loads a render, downsamples it and crops it to what is drawn.
 * @param path The image file.
 * @param supersample The factor to downsample by, at least 1.
 * @throws std::runtime_error If the factor is invalid, or the image is unreadable or blank.
 */
Cropped loadCropped(const QString& path, int supersample) {
    if (supersample < 1) {
        throw std::runtime_error("invalid supersample " + std::to_string(supersample) + " for " +
                                 path.toStdString());
    }
    const QImage raw(path);
    if (raw.isNull()) {
        throw std::runtime_error("cannot read " + path.toStdString());
    }
    const QSize size = raw.size() / supersample;
    const QImage image = raw.convertToFormat(QImage::Format_RGB32)
                             .scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    int left = size.width();
    int right = -1;
    int top = size.height();
    int bottom = -1;
    for (int y = 0; y < size.height(); ++y) {
        for (int x = 0; x < size.width(); ++x) {
            const QRgb pixel = image.pixel(x, y);
            if (qRed(pixel) < kBackgroundLevel || qGreen(pixel) < kBackgroundLevel ||
                qBlue(pixel) < kBackgroundLevel) {
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = std::max(bottom, y);
            }
        }
    }
    if (right < 0) {
        throw std::runtime_error(path.toStdString() + " is blank");
    }
    const QRect crop = QRect(QPoint(left, top), QPoint(right, bottom))
                           .adjusted(-kCropMarginPx, -kCropMarginPx, kCropMarginPx, kCropMarginPx)
                           .intersected(image.rect());
    return {.image = image.copy(crop),
            .origin = QPointF(crop.topLeft()),
            .centre = QPointF(size.width(), size.height()) / 2};
}

/**
 * @brief Draws one page. All lengths are device pixels on a page of the given size.
 */
class PageDrawer {
   public:
    /** Draws with the given painter, on a page of the given size in device pixels. */
    PageDrawer(QPainter& painter, QSizeF page) : painter_(&painter), page_(page) {}

    /** Draws everything on the page that the spec describes. */
    void draw(const QJsonObject& spec) {
        drawText(spec["title"].toString(), {0.04, 0.05}, 20, Qt::AlignLeft, true);
        drawText(spec["subtitle"].toString(), {0.04, 0.095}, 14, Qt::AlignLeft);
        drawText(spec["stamp"].toString(), {0.04, 0.975}, 8, Qt::AlignLeft, false,
                 QColor(0x55, 0x55, 0x55));

        const Cropped cropped = loadCropped(spec["image"].toString(), spec["supersample"].toInt());
        const QRectF target = imageTarget(cropped.image.size());
        painter_->drawImage(target, cropped.image);

        const double scale = target.width() / cropped.image.width();
        if (spec["scale_bar"].toBool()) {
            drawScaleBar(target, cropped.image.width(), spec["pixels_per_mm"].toDouble(), scale);
        }
        const QPointF centreOnPage = target.topLeft() + (cropped.centre - cropped.origin) * scale;
        for (const auto& label : spec["labels"].toArray()) {
            drawLabel(label.toObject(), centreOnPage, scale);
        }
        drawAxes(spec["axes"].toArray());
        drawLegend(spec["legend"].toArray());
    }

   private:
    /** Converts a position as fractions of the page to device pixels. */
    [[nodiscard]] QPointF at(QPointF fraction) const {
        return {fraction.x() * page_.width(), fraction.y() * page_.height()};
    }

    /** Fits an image into the image region, centred, keeping its aspect ratio. */
    [[nodiscard]] QRectF imageTarget(QSize image) const {
        const QRectF region(at({kImageLeft, kImageTop}), at({kImageRight, kImageBottom}));
        const double scale =
            std::min(region.width() / image.width(), region.height() / image.height());
        QRectF target(QPointF(), QSizeF(image) * scale);
        target.moveCenter(region.center());
        return target;
    }

    /** Selects the figures' font at the given size. */
    void setFont(double points, bool bold = false) {
        QFont font;
        font.setFamilies({"Ubuntu", "DejaVu Sans Mono"});
        font.setPointSizeF(points);
        font.setBold(bold);
        painter_->setFont(font);
    }

    /** Draws text at a position given as fractions of the page. */
    void drawText(const QString& text, QPointF fraction, double points, Qt::Alignment alignment,
                  bool bold = false, const QColor& colour = Qt::black) {
        setFont(points, bold);
        painter_->setPen(colour);
        drawTextAt(text, at(fraction), alignment);
    }

    /** Draws text with its baseline at a point, aligned horizontally about it. */
    void drawTextAt(const QString& text, QPointF point, Qt::Alignment alignment) {
        const double width = QFontMetricsF(painter_->font()).horizontalAdvance(text);
        double x = point.x();
        if (alignment & Qt::AlignHCenter) {
            x -= width / 2;
        } else if (alignment & Qt::AlignRight) {
            x -= width;
        }
        painter_->drawText(QPointF(x, point.y()), text);
    }

    /** Draws a black line of the given width in points. */
    void drawLine(QPointF from, QPointF to, double points) {
        QPen pen(Qt::black);
        pen.setWidthF(points * kResolutionDpi / 72);
        pen.setCapStyle(Qt::FlatCap);
        painter_->setPen(pen);
        painter_->drawLine(from, to);
    }

    /** Draws a round-numbered scale bar about a fifth of the image wide. */
    void drawScaleBar(const QRectF& target, int imageWidth, double pixelsPerMm, double scale) {
        if (pixelsPerMm <= 0) {
            throw std::runtime_error("invalid pixels_per_mm " + std::to_string(pixelsPerMm));
        }
        const double wanted = imageWidth * 0.2 / pixelsPerMm;
        const double magnitude = std::pow(10.0, std::floor(std::log10(wanted)));
        double length = magnitude;
        for (const double step : {2.0, 5.0}) {
            if (step * magnitude <= wanted) {
                length = step * magnitude;
            }
        }
        const QString label =
            length >= 1000 ? QString("%1 m").arg(length / 1000) : QString("%1 mm").arg(length);
        const double y = target.bottom() + (0.015 * page_.height());
        const QPointF right(target.right(), y);
        const QPointF left = right - QPointF(length * pixelsPerMm * scale, 0);
        drawLine(left, right, 2.5);
        setFont(12);
        painter_->setPen(Qt::black);
        drawTextAt(label, (left + right) / 2 + QPointF(0, 0.03 * page_.height()), Qt::AlignHCenter);
    }

    /** Labels a subsystem directly, so that reading the figure never relies on colour. */
    void drawLabel(const QJsonObject& label, QPointF centre, double scale) {
        const QPointF anchor =
            centre + QPointF(label["dx"].toDouble(), label["dy"].toDouble()) * scale;
        const QPointF text(anchor.x(),
                           anchor.y() - (label["row"].toInt() * 0.042 * page_.height()));
        const QString align = label["align"].toString();
        drawLine(anchor, text, 0.7);
        setFont(10.5);
        painter_->setPen(Qt::black);
        Qt::Alignment alignment = Qt::AlignHCenter;
        if (align == "left") {
            alignment = Qt::AlignLeft;
        } else if (align == "right") {
            alignment = Qt::AlignRight;
        }
        drawTextAt(label["text"].toString(), text - QPointF(0, 0.005 * page_.height()), alignment);
    }

    /** Draws x/y/z arrows as the view sees them. */
    void drawAxes(const QJsonArray& axes) {
        const QPointF centre = at({kAxesLeft + (kAxesWidth / 2), kAxesCentreY});
        const double length = kAxesWidth * page_.width() * 0.33;
        for (const auto& value : axes) {
            const QJsonObject axis = value.toObject();
            const QPointF direction(axis["dx"].toDouble(), -axis["dy"].toDouble());
            const double size = std::hypot(direction.x(), direction.y());
            setFont(12, true);
            if (size < 0.05) {
                drawAxisAlongSight(centre, axis["toward_viewer"].toBool(), length);
                painter_->setPen(Qt::black);
                setFont(12, true);
                drawTextAt(axis["name"].toString(), centre + QPointF(length * 0.3, length * 0.5),
                           Qt::AlignHCenter);
                continue;
            }
            const QPointF tip = centre + direction * length;
            drawArrow(centre, tip);
            painter_->setPen(Qt::black);
            setFont(12, true);
            // The baseline sits below the point, so that the letter is centred on it.
            const QPointF textAt =
                centre + direction * length * 1.4 + QPointF(0, 0.007 * page_.height());
            drawTextAt(axis["name"].toString(), textAt, Qt::AlignHCenter);
        }
    }

    /** Draws a line with a filled arrowhead at its end. */
    void drawArrow(QPointF from, QPointF to) {
        drawLine(from, to, 1.4);
        const QPointF along = to - from;
        const double size = std::hypot(along.x(), along.y());
        if (size == 0) {
            return;
        }
        const QPointF unit = along / size;
        const QPointF normal(-unit.y(), unit.x());
        const double head = 0.009 * page_.height();
        QPainterPath path(to);
        path.lineTo(to - unit * head * 1.6 + normal * head * 0.6);
        path.lineTo(to - unit * head * 1.6 - normal * head * 0.6);
        path.closeSubpath();
        painter_->fillPath(path, Qt::black);
    }

    /** Draws an axis pointing out of the page as a dot, and into it as a cross. */
    void drawAxisAlongSight(QPointF centre, bool towardViewer, double length) {
        const double radius = length * 0.14;
        QPen pen(Qt::black);
        pen.setWidthF(1.2 * kResolutionDpi / 72);
        painter_->setPen(pen);
        painter_->setBrush(Qt::NoBrush);
        painter_->drawEllipse(centre, radius, radius);
        if (towardViewer) {
            painter_->setBrush(Qt::black);
            painter_->drawEllipse(centre, radius * 0.3, radius * 0.3);
            painter_->setBrush(Qt::NoBrush);
        } else {
            const double arm = radius * 0.7;
            painter_->drawLine(centre + QPointF(-arm, -arm), centre + QPointF(arm, arm));
            painter_->drawLine(centre + QPointF(-arm, arm), centre + QPointF(arm, -arm));
        }
    }

    /** Draws swatches and names in rows of up to five, centred under the image. */
    void drawLegend(const QJsonArray& legend) {
        constexpr int kPerRow = 5;
        setFont(12);
        const QFontMetricsF metrics(painter_->font());
        const double swatch = 0.018 * page_.width();
        const double gap = 0.008 * page_.width();
        const double spacing = 0.022 * page_.width();
        const double rowHeight = 0.034 * page_.height();

        std::vector<double> columnWidths;
        for (qsizetype i = 0; i < legend.size(); ++i) {
            const auto column = static_cast<std::size_t>(i % kPerRow);
            const double width =
                swatch + gap + metrics.horizontalAdvance(legend[i].toObject()["label"].toString());
            if (column == columnWidths.size()) {
                columnWidths.push_back(width);
            } else {
                columnWidths[column] = std::max(columnWidths[column], width);
            }
        }
        double total =
            spacing * static_cast<double>(std::max<std::size_t>(columnWidths.size(), 1) - 1);
        for (const double width : columnWidths) {
            total += width;
        }
        const double left = ((kImageLeft + kImageRight) / 2 * page_.width()) - (total / 2);

        double x = left;
        for (qsizetype i = 0; i < legend.size(); ++i) {
            const auto column = static_cast<std::size_t>(i % kPerRow);
            if (column == 0) {
                x = left;
            }
            const qsizetype row = i / kPerRow;
            const double y = (kLegendY * page_.height()) + (static_cast<double>(row) * rowHeight);
            const QJsonObject entry = legend[i].toObject();
            const QRectF box(x, y - (swatch * 0.55), swatch, swatch * 0.55);
            QPen outline(QColor(0x22, 0x22, 0x22));
            outline.setWidthF(0.6 * kResolutionDpi / 72);
            painter_->setPen(outline);
            painter_->setBrush(QColor(entry["colour"].toString()));
            painter_->drawRect(box);
            painter_->setBrush(Qt::NoBrush);
            painter_->setPen(Qt::black);
            painter_->drawText(QPointF(x + swatch + gap, y), entry["label"].toString());
            x += columnWidths[column] + spacing;
        }
    }

    QPainter* painter_;
    QSizeF page_;
};

/**
 * @brief Writes the PDF that a spec describes.
 * @throws std::runtime_error If the spec is invalid or the file cannot be written.
 */
void writePdf(const QJsonObject& spec) {
    if (spec["output"].toString().isEmpty()) {
        throw std::runtime_error("the spec has no output file");
    }
    QPdfWriter writer(spec["output"].toString());
    writer.setTitle(spec["title"].toString());
    writer.setCreator("SHiP geometry make_figure_pdf");
    writer.setResolution(kResolutionDpi);
    writer.setPageLayout(
        QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Landscape, QMarginsF()));

    QPainter painter(&writer);
    if (!painter.isActive()) {
        throw std::runtime_error("cannot write " + spec["output"].toString().toStdString());
    }
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform |
                           QPainter::LosslessImageRendering);
    const QSizeF page(writer.width(), writer.height());
    const QJsonArray pages = spec["pages"].toArray();
    for (qsizetype i = 0; i < pages.size(); ++i) {
        if (i > 0) {
            writer.newPage();
        }
        PageDrawer(painter, page).draw(pages[i].toObject());
    }
}

/**
 * @brief Reads a spec, which has to be a JSON object.
 * @throws std::runtime_error If the file is unreadable or holds anything else.
 */
QJsonObject readSpec(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("cannot read " + path.toStdString());
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        throw std::runtime_error(path.toStdString() +
                                 " is not valid JSON: " + parseError.errorString().toStdString());
    }
    if (!document.isObject()) {
        throw std::runtime_error(path.toStdString() + " is not a JSON object");
    }
    return document.object();
}

}  // namespace

/** Writes the PDF for the spec named on the command line. */
int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    const QGuiApplication application(argc, argv);
    if (argc != 2) {
        std::cerr << "Usage: make_figure_pdf <spec.json>\n";
        return 2;
    }
    try {
        writePdf(readSpec(QString::fromLocal8Bit(argv[1])));
    } catch (const std::exception& error) {
        std::cerr << "make_figure_pdf: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
