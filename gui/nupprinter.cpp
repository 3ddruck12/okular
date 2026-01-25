/*
    SPDX-FileCopyrightText: 2026 Okular N-Up Printer
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "nupprinter.h"
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

namespace Okular {

NUpPrinter::NUpPrinter(QObject *parent)
    : QObject(parent)
{
}

NUpPrinter::~NUpPrinter()
{
}

bool NUpPrinter::generateNUpPdf(Okular::Document *doc, const QString &outPath, const NUpOptions &opts)
{
    if (!doc || opts.pagesPerSheet <= 0) {
        return false;
    }

    QPdfWriter writer(outPath);
    writer.setCreator(QStringLiteral("Okular N-Up Printer"));
    
    QPageLayout layout;
    layout.setPageSize(QPageSize(QPageSize::A4));
    layout.setOrientation(QPageLayout::Portrait);
    
    int rows = 1;
    int cols = 1;
    
    switch (opts.pagesPerSheet) {
        case 2: rows = 2; cols = 1; break;
        case 4: rows = 2; cols = 2; break;
        case 6: rows = 3; cols = 2; break;
        case 9: rows = 3; cols = 3; break;
        case 16: rows = 4; cols = 4; break;
        default: 
            cols = std::ceil(std::sqrt(opts.pagesPerSheet));
            rows = std::ceil((double)opts.pagesPerSheet / cols);
            break;
    }
    
    writer.setPageLayout(layout);
    writer.setResolution(300);

    QPainter painter;
    if (!painter.begin(&writer)) {
        return false;
    }

    int docPages = doc->pages();
    int sheets = std::ceil((double)docPages / opts.pagesPerSheet);
    
    double mmToDots = 300.0 / 25.4;
    double sheetW = layout.pageSize().size(QPageSize::Millimeter).width() * mmToDots;
    double sheetH = layout.pageSize().size(QPageSize::Millimeter).height() * mmToDots;
    
    double cellW = sheetW / cols;
    double cellH = sheetH / rows;

    for (int s = 0; s < sheets; ++s) {
        if (s > 0) writer.newPage();
        
        for (int i = 0; i < opts.pagesPerSheet; ++i) {
            int pageIndex = s * opts.pagesPerSheet + i;
            if (pageIndex >= docPages) break;
            
            int r = 0, c = 0;
            switch (opts.pageOrder) {
                case 0: // Horizontal
                    r = i / cols;
                    c = i % cols;
                    break;
                case 1: // Horizontal Reversed
                    r = i / cols;
                    c = cols - 1 - (i % cols);
                    break;
                case 2: // Vertical
                    c = i / rows;
                    r = i % rows;
                    break;
                case 3: // Vertical Reversed
                    c = cols - 1 - (i / rows);
                    r = i % rows;
                    break;
            }

            const Okular::Page *page = doc->page(pageIndex);
            
            double pW = page->width();
            double pH = page->height();
            double pRatio = pW / pH;
            double cellRatio = cellW / cellH;
            
            double renderW, renderH;
            if (pRatio > cellRatio) {
                renderW = cellW;
                renderH = cellW / pRatio;
            } else {
                renderH = cellH;
                renderW = cellH * pRatio;
            }
            
            double offX = (cellW - renderW) / 2.0;
            double offY = (cellH - renderH) / 2.0;
            
            double destX = c * cellW + offX;
            double destY = r * cellH + offY;
            
            QRect targetRectDots(std::round(destX), std::round(destY), std::round(renderW), std::round(renderH));
            
            int flags = PagePainter::Accessibility | PagePainter::Highlights | PagePainter::Annotations;
            PagePainter::paintPageOnPainter(&painter, page, nullptr, flags, 
                                            std::round(renderW), std::round(renderH), 
                                            targetRectDots);
            
            if (opts.drawBorder) {
                painter.save();
                painter.setPen(Qt::black);
                painter.drawRect(targetRectDots);
                painter.restore();
            }
        }
    }
    
    painter.end();
    return true;
}

}
