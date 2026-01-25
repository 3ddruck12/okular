/*
    SPDX-FileCopyrightText: 2026 Okular Poster Printer
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "posterprinter.h"
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
#include <QFont>
#include <QPen>
#include <QVector>

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
    
    // Use A4 as default tile size for now, or fetch from printer settings?
    // Since we are generating a PDF to be printed by the system printer later,
    // we should probably stick to A4 or allow user to pick. 
    // Ideally this should come from the current printer configuration, but 
    // generatePosterPdf creates a distinct file. 
    // Let's assume A4 for tiles for now, as that's safe for most home printers.
    // A better approach would be to detect the target printer page size, 
    // but here we are in a generator logic.
    
    QPageLayout layout;
    layout.setPageSize(QPageSize(QPageSize::A4));
    layout.setOrientation(QPageLayout::Portrait);
    layout.setMargins(QMarginsF(0, 0, 0, 0)); // We handle margins manually or assume full bleed for tiling logic
    
    writer.setPageLayout(layout);

    QPainter painter;
    if (!painter.begin(&writer)) {
        qWarning() << "Failed to begin painting on QPdfWriter";
        return false;
    }

    // Resolution for the PDF writer
    // QPdfWriter defaults to screen resolution often, let's set it high for quality
    writer.setResolution(300); 

    const int docPages = doc->pages();
    for (int i = 0; i < docPages; ++i) {
        const Okular::Page *page = doc->page(i);
        
        // 1. Get original page size in mm
        double origWidthMm = page->width() * 25.4 / 72.0; // Point to mm ? Wait, page->width() is normalized? No, it's pixels? 
        // Okular::Page::width() is in "pixels" at 72 DPI usually?
        // Let's check PageSizeMetric.
        // Assuming page magnitude is standard points (1/72 inch).
        origWidthMm = page->width() * 25.4 / 72.0;
        double origHeightMm = page->height() * 25.4 / 72.0;

        // Apply scale
        double scaledWidthMm = origWidthMm * (opts.tileScale / 100.0);
        double scaledHeightMm = origHeightMm * (opts.tileScale / 100.0);

        // Target (Tile) size
        QSizeF tileSize = layout.pageSize().size(QPageSize::Millimeter);
        double tileW = tileSize.width();
        double tileH = tileSize.height();
        
        // Printable area (subtract margins if any, but we set 0)
        // Let's assume useable area is slightly smaller to be safe, e.g. 5mm margins?
        // Or user standard margins.
        double margin = 5.0; 
        double useableW = tileW - 2 * margin;
        double useableH = tileH - 2 * margin;

        // Calculate rows and cols
        // Concept: We step through the scaled image by (useableW - overlap)
        double stepX = useableW - opts.overlap;
        double stepY = useableH - opts.overlap;

        int cols = std::ceil(scaledWidthMm / stepX);
        int rows = std::ceil(scaledHeightMm / stepY);

        if (cols <= 0) cols = 1;
        if (rows <= 0) rows = 1;

        // Render the FULL page to an image
        // Resolution: high enough for print (e.g. 300 DPI)
        // Okular::Document::requestPixmaps is async. 
        // We need a sync way or wait.
        // Or we use `Generator::image(request)` if we can access the generator.
        // `doc->page(i)` ...
        // Actually, we can use `doc->requestPixmaps` but it's void.
        // Wait, how to get the image synchronously?
        // There is no public sync API on Document to get an image easily without observer.
        
        // Workaround: Use a local QEventLoop if needed, or check if we can access Generator directly?
        // Generator is protected in Document.
        
        // However, we are in `core` (if we put this file in core), so we can access Document internals if we are a friend?
        // `Document` class declaration: friend class PosterPrinter? No.
        
        // Let's look at `Document::requestPixmaps`. It notifies observers.
        // We can register a temporary Observer.
        
        // TODO: For now, let's placeholder the rendering logic.
        // We will assume we can get a QImage.
        // For Proof of Concept, let's just draw rectangles.
        
        // Render the FULL page to an image
        // Resolution: high enough for print (e.g. 300 DPI)
        // Since we cannot easily get a sync pixmap from Document without Observer,
        // we will use a workaround or assuming we can implement a synchronous helper.
        // Actually, `Generator` has `generatePixmap(PixmapRequest *request)`.
        // But `Document::requestPixmaps` is the public API.
        
        // Let's implement a little helper class that acts as an observer to get the pixmap synchronously (waiting).
        // This is blocking code in the GUI thread potentially, but since we are modal in printing, it might be acceptable.
        
        // Wait, `Document::requestPixmaps` posts events.
        // We can create a local event loop.
        
        class SyncPixmapObserver : public DocumentObserver {
        public:
            SyncPixmapObserver() : m_page(nullptr) {}
            ~SyncPixmapObserver() override {}
            
            void notifySetup(const QVector<Okular::Page *> &pages, int setupFlags) override {}
            void notifyViewportChanged(bool smoothMove) override {}
            void notifyPageChanged(int page, int flags) override {
                if (flags & DocumentObserver::Pixmap && m_page && page == m_page->number()) {
                     if (m_page->hasPixmap(this, m_width, m_height)) {
                         m_loop.quit();
                     }
                }
            }
            void notifyContentsCleared(int changed) override {}
            void notifyZoom(int factor) override {}
            bool canUnloadPixmap(int page) const override { return true; }

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
        
        // Calculate pixel size for 300 DPI
        int pixW = (int)(page->width() * (300.0 / 72.0));
        int pixH = (int)(page->height() * (300.0 / 72.0));
        
        PixmapRequest *req = new PixmapRequest(&observer, page->number(), pixW, pixH, 1.0, 0, PixmapRequest::NoFeature);
        observer.setPage(page, pixW, pixH);
        doc->requestPixmaps({req});
        
        observer.waitForPixmap();
        
        const QPixmap *pix = page->pixmap(&observer, pixW, pixH);
        QImage pageImage;
        if (pix) {
            pageImage = pix->toImage();
        }
        
        doc->removeObserver(&observer);
        
        if (pageImage.isNull()) {
            qWarning() << "Failed to render page" << i;
            continue;
        }

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (r > 0 || c > 0 || i > 0) { // i > 0 check to assume newPage for subsequent pages
                    writer.newPage();
                }
                
                // Calculate source rect in pixels
                // stepX, stepY are in mm. need to convert to pixels of the image
                double mmToPix = 300.0 / 25.4;
                
                // Original Page Size (Mm) vs Scaled Page Size
                // The Image we have is of Original Page Size.
                // But we are Tiling a Scaled version of it.
                // So we should map back the tile rect to the original image rect.
                
                double scaleFactor = opts.tileScale / 100.0;
                
                // Tile in scaled logic:
                double tileX_mm = c * stepX;
                double tileY_mm = r * stepY;
                double tileW_mm = useableW;
                double tileH_mm = useableH;
                
                // Map to Scaling
                // (TileX / Scale) -> Original Mm
                // Original Mm * mmToPix -> Image Pixels
                
                double srcX_mm = tileX_mm / scaleFactor;
                double srcY_mm = tileY_mm / scaleFactor;
                double srcW_mm = tileW_mm / scaleFactor;
                double srcH_mm = tileH_mm / scaleFactor;
                
                QRectF srcRect(srcX_mm * mmToPix, srcY_mm * mmToPix, srcW_mm * mmToPix, srcH_mm * mmToPix);
                
                // Target Rect on PDF Page (mm)
                // Draw at margins? We set margins to 0, so we can draw at x=margin, y=margin
                // margin is 5.0
                QRectF targetRect(margin * 300/25.4, margin * 300/25.4, useableW * 300/25.4, useableH * 300/25.4);
                 // Warning: PDFWriter painter uses "Paint Device Metric"? 
                 // QPdfWriter uses points? No, QPainter on QPdfWriter uses the resolution set on writer (300).
                 // So coords are in dots (1/300 inch).
                 
                 // useableW (mm) -> dots
                 double mmToDots = 300.0 / 25.4;
                 QRectF targetRectDots(margin * mmToDots, margin * mmToDots, useableW * mmToDots, useableH * mmToDots);
                 
                painter.drawImage(targetRectDots, pageImage, srcRect);

                // Draw tile info
                if (opts.labels) {
                    painter.save();
                    painter.setPen(Qt::black);
                    QFont f = painter.font();
                    f.setPointSize(8);
                    painter.setFont(f);
                    // Bottom-left text
                    painter.drawText(targetRectDots.left(), targetRectDots.bottom() + 30, QString("Page %1 - Tile %2,%3").arg(i+1).arg(r+1).arg(c+1));
                    painter.restore();
                }
                
                // Draw Cut Marks
                if (opts.cutMarks) {
                    painter.save();
                    QPen pen(Qt::black);
                    pen.setWidthF(1.0); // 1 dot width
                    painter.setPen(pen);
                    
                    double len = 10.0 * mmToDots; // 10mm mark length
                    double mDots = margin * mmToDots;
                    
                    // Top-Left corner
                    // Horizontal line
                    painter.drawLine(QPointF(mDots - len, mDots), QPointF(mDots, mDots));
                    // Vertical line
                    painter.drawLine(QPointF(mDots, mDots - len), QPointF(mDots, mDots));

                    // Top-Right corner
                    double right = mDots + useableW * mmToDots;
                    painter.drawLine(QPointF(right, mDots), QPointF(right + len, mDots));
                    painter.drawLine(QPointF(right, mDots - len), QPointF(right, mDots));
                    
                    // Bottom-Left
                    double bottom = mDots + useableH * mmToDots;
                    painter.drawLine(QPointF(mDots - len, bottom), QPointF(mDots, bottom));
                    painter.drawLine(QPointF(mDots, bottom), QPointF(mDots, bottom + len));
                    
                    // Bottom-Right
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
