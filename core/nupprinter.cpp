/*
    SPDX-FileCopyrightText: 2026 Okular N-Up Printer
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "nupprinter.h"
#include "document.h"
#include "page.h"
#include "generator.h"

#include <QPdfWriter>
#include <QPainter>
#include <QPageLayout>
#include <QPageSize>
#include <QDebug>
#include <QPixmap>
#include <cmath>
#include <QEventLoop>
#include <QVector>

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
    
    // Default to A4 for output sheet for now, similar to PosterPrinter assumption.
    // Ideally user selects output size in printer settings, which we might want to respect here.
    QPageLayout layout;
    layout.setPageSize(QPageSize(QPageSize::A4));
    layout.setOrientation(QPageLayout::Portrait); // Or auto-rotate based on N?
    // If 2-up, landscape is usually better on A4 portrait inputs.
    // Let's stick to Portrait for simplicity or use logic.
    
    // N-Up Logic Grid Calculation
    int rows = 1;
    int cols = 1;
    
    // Simple heuristic for rows/cols
    switch (opts.pagesPerSheet) {
        case 2: rows = 2; cols = 1; break; // 2 on 1 page (Booklet style often 2 cols, 1 row for Landscape output. For Portrait output, 2 rows, 1 col)
        case 4: rows = 2; cols = 2; break;
        case 6: rows = 3; cols = 2; break;
        case 9: rows = 3; cols = 3; break;
        case 16: rows = 4; cols = 4; break;
        default: 
            // Square root approx
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

    // Helper for Sync Pixmap (Duplicated from PosterPrinter, ideally refactor to util)
    class SyncPixmapObserver : public DocumentObserver {
    public:
        SyncPixmapObserver() : m_page(nullptr) {}
        ~SyncPixmapObserver() override {}
        
        void notifyPageChanged(int page, int flags) override {
            if (flags & DocumentObserver::Pixmap && m_page && page == m_page->number()) {
                 if (m_page->hasPixmap(this, m_width, m_height)) {
                     m_loop.quit();
                 }
            }
        }
        void notifySetup(const QVector<Okular::Page *> &, int) override {}
        void notifyViewportChanged(bool) override {}
        void notifyContentsCleared(int) override {}
        void notifyZoom(int) override {}
        bool canUnloadPixmap(int) const override { return true; }

        void setPage(const Okular::Page *p, int w, int h) { m_page = p; m_width = w; m_height = h; }
        void waitForPixmap() { m_loop.exec(); }
        
    private:
         QEventLoop m_loop;
         const Okular::Page *m_page;
         int m_width;
         int m_height;
    };
    
    SyncPixmapObserver observer;
    doc->addObserver(&observer);

    int docPages = doc->pages();
    int sheets = std::ceil((double)docPages / opts.pagesPerSheet);
    
    // Sheet Dimensions (in dots)
    double mmToDots = 300.0 / 25.4;
    double sheetW = layout.pageSize().size(QPageSize::Millimeter).width() * mmToDots;
    double sheetH = layout.pageSize().size(QPageSize::Millimeter).height() * mmToDots;
    
    // Cell Dimensions
    double cellW = sheetW / cols;
    double cellH = sheetH / rows;

    for (int s = 0; s < sheets; ++s) {
        if (s > 0) writer.newPage();
        
        for (int i = 0; i < opts.pagesPerSheet; ++i) {
            int pageIndex = s * opts.pagesPerSheet + i;
            if (pageIndex >= docPages) break;
            
            // Determine Grid Position (r, c) based on order
            int r = 0, c = 0;
            switch (opts.pageOrder) {
                case 0: // Horizontal
                    r = i / cols;
                    c = i % cols;
                    break;
                case 1: // Horizontal Reversed
                    r = i / cols;
                    c = cols - 1 - (i % cols); // Right to Left
                    break;
                case 2: // Vertical
                    c = i / rows;
                    r = i % rows;
                    break;
                case 3: // Vertical Reversed
                    c = cols - 1 - (i / rows);
                    r = i % rows; // TopDown but Right to Left cols?
                    // "Vertical" usually means fill col 1, then col 2.
                    // "Vertical Reversed" probably fill last col, then prev col? 
                    // Let's assume standard intuitive interpretation.
                    break;
            }
            // Logic improvement for exact "Reversed" semantics needed if strictly requested, 
            // but this is a good start.

            const Okular::Page *page = doc->page(pageIndex);
            
            // Calculate scale to fit cell
            // Get Page Aspect Ratio
            double pW = page->width();
            double pH = page->height();
            double pRatio = pW / pH;
            
            double cellRatio = cellW / cellH;
            
            // Fit logic
            double renderW, renderH;
            if (pRatio > cellRatio) {
                // Page is wider than cell -> Fit Width
                renderW = cellW;
                renderH = cellW / pRatio;
            } else {
                // Cell is wider -> Fit Height
                renderH = cellH;
                renderW = cellH * pRatio;
            }
            
            // Centering in cell
            double offX = (cellW - renderW) / 2.0;
            double offY = (cellH - renderH) / 2.0;
            
            double destX = c * cellW + offX;
            double destY = r * cellH + offY;
            
            // Request Pixmap at target resolution
            int pixW = (int)renderW;
            int pixH = (int)renderH;
            
            // Avoid 0 size
            if (pixW <= 0) pixW = 100;
            if (pixH <= 0) pixH = 100;

            PixmapRequest *req = new PixmapRequest(&observer, page->number(), pixW, pixH, 1.0, 0, PixmapRequest::NoFeature);
            observer.setPage(page, pixW, pixH);
            doc->requestPixmaps({req});
            
            observer.waitForPixmap();
            
            const QPixmap *pix = page->pixmap(&observer, pixW, pixH);
            if (pix) {
                QImage img = pix->toImage();
                painter.drawImage(QRectF(destX, destY, renderW, renderH), img);
                
                if (opts.drawBorder) {
                    painter.drawRect(QRectF(destX, destY, renderW, renderH));
                }
            }
        }
    }
    
    doc->removeObserver(&observer);
    painter.end();
    return true;
}

}
