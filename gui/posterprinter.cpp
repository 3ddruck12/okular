/*
    SPDX-FileCopyrightText: 2026 Okular Poster Printer
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "posterprinter.h"
#include "core/document.h"
#include "core/page.h"
#include "core/generator.h"
#include "core/observer.h"
#include "gui/pagepainter.h"

#include <QPdfWriter>
#include <QPainter>
#include <QPageLayout>
#include <QPageSize>
#include <QDebug>
#include <cmath>
#include <QFont>
#include <QPen>

namespace Okular {

PosterPrinter::PosterPrinter(QObject *parent)
    : QObject(parent)
{
}

PosterPrinter::~PosterPrinter()
{
}

bool PosterPrinter::generatePosterPdf(Okular::Document *doc, const QString &outPath, const PosterOptions &opts)
{
    if (!doc) {
        return false;
    }

    QPdfWriter writer(outPath);
    writer.setCreator(QStringLiteral("Okular Poster Printer"));
    
    QPageLayout layout;
    layout.setPageSize(QPageSize(QPageSize::A4));
    layout.setOrientation(QPageLayout::Portrait);
    layout.setMargins(QMarginsF(0, 0, 0, 0));
    
    writer.setPageLayout(layout);

    QPainter painter;
    if (!painter.begin(&writer)) {
        qWarning() << "Failed to begin painting on QPdfWriter";
        return false;
    }

    writer.setResolution(300); 

    const int docPages = doc->pages();
    for (int i = 0; i < docPages; ++i) {
        const Okular::Page *page = doc->page(i);
        
        // Original page size in mm
        double origWidthMm = page->width() * 25.4 / 72.0;
        double origHeightMm = page->height() * 25.4 / 72.0;

        // Apply scale
        double scaledWidthMm = origWidthMm * (opts.tileScale / 100.0);
        double scaledHeightMm = origHeightMm * (opts.tileScale / 100.0);

        // Target (Tile) size
        QSizeF tileSize = layout.pageSize().size(QPageSize::Millimeter);
        double tileW = tileSize.width();
        double tileH = tileSize.height();
        
        double margin = 5.0; 
        double useableW = tileW - 2 * margin;
        double useableH = tileH - 2 * margin;

        double stepX = useableW - opts.overlap;
        double stepY = useableH - opts.overlap;

        int cols = std::ceil(scaledWidthMm / stepX);
        int rows = std::ceil(scaledHeightMm / stepY);

        if (cols <= 0) cols = 1;
        if (rows <= 0) rows = 1;

        double mmToDots = 300.0 / 25.4;

        // Full scaled size in dots for PagePainter
        int fullScaledWidthDots = std::round(scaledWidthMm * mmToDots);
        int fullScaledHeightDots = std::round(scaledHeightMm * mmToDots);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (r > 0 || c > 0 || i > 0) {
                    writer.newPage();
                }
                
                // Crop rect in mm (relative to scaled page)
                double tileX_mm = c * stepX;
                double tileY_mm = r * stepY;
                double tileW_mm = std::min(useableW, scaledWidthMm - tileX_mm);
                double tileH_mm = std::min(useableH, scaledHeightMm - tileY_mm);
                
                // Convert to NormalizedRect
                Okular::NormalizedRect crop(tileX_mm / scaledWidthMm, 
                                            tileY_mm / scaledHeightMm, 
                                            (tileX_mm + tileW_mm) / scaledWidthMm, 
                                            (tileY_mm + tileH_mm) / scaledHeightMm);
                
                // target rectangle on PDF page in dots
                QRect targetRectDots(std::round(margin * mmToDots), 
                                     std::round(margin * mmToDots), 
                                     std::round(tileW_mm * mmToDots), 
                                     std::round(tileH_mm * mmToDots));
                
                // Paint using PagePainter
                int flags = PagePainter::Accessibility | PagePainter::Highlights | PagePainter::Annotations;
                PagePainter::paintCroppedPageOnPainter(&painter, page, nullptr, flags, 
                                                       fullScaledWidthDots, fullScaledHeightDots, 
                                                       targetRectDots, crop, nullptr);

                // Draw tile info (fixed QString constructor)
                if (opts.labels) {
                    painter.save();
                    painter.setPen(Qt::black);
                    QFont f = painter.font();
                    f.setPointSize(8);
                    painter.setFont(f);
                    painter.drawText(targetRectDots.left(), targetRectDots.bottom() + 30, 
                                     QStringLiteral("Page %1 - Tile %2,%3").arg(i+1).arg(r+1).arg(c+1));
                    painter.restore();
                }
                
                // Draw Cut Marks
                if (opts.cutMarks) {
                    painter.save();
                    QPen pen(Qt::black);
                    pen.setWidthF(1.0); 
                    painter.setPen(pen);
                    
                    double len = 10.0 * mmToDots; 
                    double mDots = margin * mmToDots;
                    
                    painter.drawLine(QPointF(mDots - len, mDots), QPointF(mDots, mDots));
                    painter.drawLine(QPointF(mDots, mDots - len), QPointF(mDots, mDots));

                    double right = mDots + tileW_mm * mmToDots;
                    painter.drawLine(QPointF(right, mDots), QPointF(right + len, mDots));
                    painter.drawLine(QPointF(right, mDots - len), QPointF(right, mDots));
                    
                    double bottom = mDots + tileH_mm * mmToDots;
                    painter.drawLine(QPointF(mDots - len, bottom), QPointF(mDots, bottom));
                    painter.drawLine(QPointF(mDots, bottom), QPointF(mDots, bottom + len));
                    
                    painter.drawLine(QPointF(right, bottom), QPointF(right + len, bottom));
                    painter.drawLine(QPointF(right, bottom), QPointF(right, bottom + len));
                    
                    painter.restore();
                }
            }
        }
    }

    painter.end();
    return true;
}

}
