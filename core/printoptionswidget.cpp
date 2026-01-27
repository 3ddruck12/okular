/*
    SPDX-FileCopyrightText: 2019 Michael Weghorn <m.weghorn@posteo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "printoptionswidget.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QPainter>
#include <QPen>

#include "document.h"
#include "page.h"
#include <KLocalizedString>

namespace Okular
{
// --- PosterPreviewWidget ---

class PosterPreviewWidget : public QWidget
{
public:
    explicit PosterPreviewWidget(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_doc(nullptr)
        , m_tileScale(100.0)
        , m_overlap(0.0)
        , m_cutMarks(false)
        , m_labels(false)
    {
        setMinimumSize(300, 300);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    
    void setDocument(Okular::Document *doc) {
        m_doc = doc;
        update();
    }

    void setOptions(double scale, double overlap, bool cutMarks, bool labels) {
        m_tileScale = scale;
        m_overlap = overlap;
        m_cutMarks = cutMarks;
        m_labels = labels;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Draw background
        painter.fillRect(rect(), QColor(240, 240, 240));

        if (!m_doc) {
            painter.drawText(rect(), Qt::AlignCenter, i18n("No document loaded"));
            return;
        }

        // Use first page for preview
        const Okular::Page *page = m_doc->page(0);
        if (!page) return;

        double origW = page->width();
        double origH = page->height();

        if (origW <= 0 || origH <= 0) return;

        // Calculate fitted rect centered in widget
        double aspect = origW / origH;
        double widgetAspect = (double)width() / height();
        
        double drawW, drawH;
        if (aspect > widgetAspect) {
             drawW = width() * 0.8;
             drawH = drawW / aspect;
        } else {
             drawH = height() * 0.8;
             drawW = drawH * aspect;
        }
        
        double offsetX = (width() - drawW) / 2.0;
        double offsetY = (height() - drawH) / 2.0;
        
        QRectF pageRect(offsetX, offsetY, drawW, drawH);

        // Draw Page Shadow
        painter.fillRect(pageRect.translated(5, 5), QColor(0, 0, 0, 50));
        // Draw Page Background
        painter.fillRect(pageRect, Qt::white);
        painter.setPen(Qt::black);
        painter.drawRect(pageRect);
        
        // Draw Text "Page 1 Preview"
        painter.setPen(Qt::gray);
        painter.drawText(pageRect, Qt::AlignCenter, i18n("Page 1"));

        // Now draw Poster Grid
        // 1. Calculate dimensions in mm
        double origWidthMm = origW * 25.4 / 72.0;
        double origHeightMm = origH * 25.4 / 72.0;
        
        double scaledWidthMm = origWidthMm * (m_tileScale / 100.0);
        double scaledHeightMm = origHeightMm * (m_tileScale / 100.0);

        // A4 Paper Assume (standard for simple preview)
        // TODO: Get actual paper size from printer settings if possible, but A4 is safe default for preview
        double paperW = 210.0; 
        double paperH = 297.0; 
        
        double margin = 5.0;
        double useableW = paperW - 2 * margin;
        double useableH = paperH - 2 * margin;

        double stepX = useableW - m_overlap;
        double stepY = useableH - m_overlap;

        int cols = std::ceil(scaledWidthMm / stepX);
        int rows = std::ceil(scaledHeightMm / stepY);

        // Map mm to preview pixels
        // The pageRect represents 'origWidthMm' (unscaled)
        // BUT poster mode scales the CONTENT. The page size itself effectively becomes 'scaledWidthMm'
        // Wait, "Tile Scale" scales the page content UP. The physical paper stays same (A4).
        // So we need to draw a grid over a rectangle that represents the SCALED content.
        
        // Let's visualize the SCALED content rect.
        // If scale is 200%, the content rect is 2x bigger than pageRect?
        // No, usually we fit the visible content to the view.
        // Let's treat 'pageRect' as the Scaled Content Area.
        
        // Scale factor from mm to pixels in preview
        double mmToPix = drawW / scaledWidthMm; 

        painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
        
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                 double x_mm = c * stepX;
                 double y_mm = r * stepY;
                 
                 // Tile rect in mm logic
                 double tW_mm = std::min(useableW, scaledWidthMm - x_mm);
                 double tH_mm = std::min(useableH, scaledHeightMm - y_mm);
                 
                 QRectF tileRect(pageRect.left() + x_mm * mmToPix, 
                                 pageRect.top() + y_mm * mmToPix,
                                 tW_mm * mmToPix,
                                 tH_mm * mmToPix);
                 
                 painter.drawRect(tileRect);
                 
                 if (m_labels) {
                     // Draw small label
                     QFont f = painter.font();
                     f.setPointSize(6); // manual small font
                     painter.setFont(f);
                     painter.drawText(tileRect, Qt::AlignCenter, QStringLiteral("%1,%2").arg(r+1).arg(c+1));
                 }
                 
                 // Overlap visualization (shaded area at right/bottom of tile if not last)
                 if (m_overlap > 0.0) {
                     painter.save();
                     QColor ovCol(255, 0, 0, 100);
                     // If there IS an overlap to the right (cols > c+1)
                     // Using stepX, overlap is part of the step? 
                     // No, stepX = useableW - overlap.
                     // The tile is 'useableW' wide.
                     // The next tile starts at 'x + stepX'.
                     // So the overlap region is [x+stepX, x+useableW].
                     
                     if (c < cols - 1) {
                         double overlapStart = x_mm + stepX;
                         double overlapW = m_overlap;
                         QRectF ovRect(pageRect.left() + overlapStart * mmToPix,
                                       tileRect.top(),
                                       overlapW * mmToPix,
                                       tileRect.height()); // full height of tile
                         painter.fillRect(ovRect, ovCol);
                     }
                     if (r < rows - 1) {
                         double overlapStart = y_mm + stepY;
                         double overlapH = m_overlap;
                         QRectF ovRect(tileRect.left(),
                                       pageRect.top() + overlapStart * mmToPix,
                                       tileRect.width(),
                                       overlapH * mmToPix);
                         painter.fillRect(ovRect, ovCol);
                     }
                     painter.restore();
                 }
            }
        }
    }

private:
    Okular::Document *m_doc;
    double m_tileScale;
    double m_overlap;
    bool m_cutMarks;
    bool m_labels;
};

// --- DefaultPrintOptionsWidget Implementation ---

DefaultPrintOptionsWidget::DefaultPrintOptionsWidget(QWidget *parent, Okular::Document *doc)
    : PrintOptionsWidget(parent)
{
    setWindowTitle(i18n("Print Options"));
    
    // Main HLayout: Left Settings, Right Preview
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    
    // Container for settings (Left side)
    QWidget *settingsContainer = new QWidget(this);
    QFormLayout *layout = new QFormLayout(settingsContainer);
    
    // Print Mode Selection
    layout->addRow(i18n("Print Mode:"), m_printModeCombo = new QComboBox(this));
    m_printModeCombo->addItem(i18n("Fit to printable area"), QVariant::fromValue(PrintMode::FitToPrintableArea));
    m_printModeCombo->addItem(i18n("Fit to full page"), QVariant::fromValue(PrintMode::FitToPage));
    m_printModeCombo->addItem(i18n("Poster"), QVariant::fromValue(PrintMode::Poster));
    m_printModeCombo->addItem(i18n("Multiple pages per sheet"), QVariant::fromValue(PrintMode::MultiplePages));

    // Mode Stack
    m_modeStack = new QStackedWidget(this);
    layout->addRow(m_modeStack);
    
    // Page 0 & 1: No extra options (Empty widget)
    m_modeStack->addWidget(new QWidget(this));
    m_modeStack->addWidget(new QWidget(this));

    // Page 2: Poster Options
    QWidget *posterWidget = new QWidget(this);
    QFormLayout *posterLayout = new QFormLayout(posterWidget);
    posterLayout->setContentsMargins(0, 0, 0, 0);

    m_posterTileScale = new QDoubleSpinBox(this);
    m_posterTileScale->setRange(10.0, 1000.0);
    m_posterTileScale->setValue(100.0);
    m_posterTileScale->setSuffix(i18n("%"));
    posterLayout->addRow(i18n("Tile Scale:"), m_posterTileScale);

    m_posterOverlap = new QDoubleSpinBox(this);
    m_posterOverlap->setRange(0.0, 100.0);
    m_posterOverlap->setValue(10.0); 
    m_posterOverlap->setSuffix(i18n(" mm"));
    posterLayout->addRow(i18n("Overlap:"), m_posterOverlap);

    m_posterCutMarks = new QCheckBox(i18n("Draw Cut Marks"), this);
    m_posterCutMarks->setChecked(true);
    posterLayout->addRow(QString(), m_posterCutMarks);
    
    m_posterLabels = new QCheckBox(i18n("Draw Labels"), this);
    m_posterLabels->setChecked(true);
    posterLayout->addRow(QString(), m_posterLabels);

    m_modeStack->addWidget(posterWidget);

    // Page 3: N-Up Options
    QWidget *nUpWidget = new QWidget(this);
    QFormLayout *nUpLayout = new QFormLayout(nUpWidget);
    nUpLayout->setContentsMargins(0, 0, 0, 0);

    m_nUpPagesPerSheet = new QComboBox(this);
    m_nUpPagesPerSheet->addItem(i18n("2"), 2);
    m_nUpPagesPerSheet->addItem(i18n("4"), 4);
    m_nUpPagesPerSheet->addItem(i18n("6"), 6);
    m_nUpPagesPerSheet->addItem(i18n("9"), 9);
    m_nUpPagesPerSheet->addItem(i18n("16"), 16);
    nUpLayout->addRow(i18n("Pages per sheet:"), m_nUpPagesPerSheet);
    
    m_nUpPageOrder = new QComboBox(this);
    m_nUpPageOrder->addItem(i18n("Horizontal"), 0);
    m_nUpPageOrder->addItem(i18n("Horizontal Reversed"), 1);
    m_nUpPageOrder->addItem(i18n("Vertical"), 2);
    m_nUpPageOrder->addItem(i18n("Vertical Reversed"), 3);
    nUpLayout->addRow(i18n("Page Order:"), m_nUpPageOrder);

    m_nUpDrawBorder = new QCheckBox(i18n("Print Page Border"), this);
    nUpLayout->addRow(QString(), m_nUpDrawBorder);
    
    m_modeStack->addWidget(nUpWidget);

    // Initial state
    m_printModeCombo->setCurrentIndex(0);
    m_modeStack->setCurrentIndex(0);
    
    // Add Settings to Main Layout
    mainLayout->addWidget(settingsContainer, 0, Qt::AlignTop);

    // Preview Widget (Right side)
    m_posterPreview = new PosterPreviewWidget(this);
    m_posterPreview->setDocument(doc);
    mainLayout->addWidget(m_posterPreview, 1); // Expand to take remaining space
    
    // Only show preview when Poster mode is selected
    m_posterPreview->setVisible(false);

    connect(m_printModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DefaultPrintOptionsWidget::slotPrintModeChanged);
    
    // Connect Poster Options to Preview update
    connect(m_posterTileScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &DefaultPrintOptionsWidget::slotPosterOptionChanged);
    connect(m_posterOverlap, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &DefaultPrintOptionsWidget::slotPosterOptionChanged);
    connect(m_posterCutMarks, &QCheckBox::toggled, this, &DefaultPrintOptionsWidget::slotPosterOptionChanged);
    connect(m_posterLabels, &QCheckBox::toggled, this, &DefaultPrintOptionsWidget::slotPosterOptionChanged);
}

void DefaultPrintOptionsWidget::slotPrintModeChanged(int index)
{
    // Map combo index to stack index
    // Combo: 0 (FitArea), 1 (FitPage), 2 (Poster), 3 (Multiple)
    // Stack Index 0: Empty (for FitArea)
    // Stack Index 1: Empty (for FitPage)
    // Stack Index 2: Poster
    // Stack Index 3: N-Up
    
    if (index >= 0 && index < m_modeStack->count()) {
        m_modeStack->setCurrentIndex(index);
    }
    
    // Toggle Preview Visibility
    if (index == 2) { // Poster
        m_posterPreview->setVisible(true);
        slotPosterOptionChanged(); // Update initial state
    } else {
        m_posterPreview->setVisible(false);
    }
}

void DefaultPrintOptionsWidget::slotPosterOptionChanged()
{
    m_posterPreview->setOptions(m_posterTileScale->value(),
                                m_posterOverlap->value(),
                                m_posterCutMarks->isChecked(),
                                m_posterLabels->isChecked());
}


PrintOptionsWidget::PrintMode DefaultPrintOptionsWidget::printMode() const
{
    return m_printModeCombo->currentData().value<PrintMode>();
}

// N-Up accessors
int DefaultPrintOptionsWidget::nUpPagesPerSheet() const
{
    return m_nUpPagesPerSheet->currentData().toInt();
}

int DefaultPrintOptionsWidget::nUpPageOrder() const
{
    return m_nUpPageOrder->currentData().toInt();
}

bool DefaultPrintOptionsWidget::nUpDrawBorder() const
{
    return m_nUpDrawBorder->isChecked();
}

// Legacy / Mappings
bool DefaultPrintOptionsWidget::ignorePrintMargins() const
{
    return printMode() == FitToPage;
}

// Poster Accessors
double DefaultPrintOptionsWidget::posterTileScale() const
{
    return m_posterTileScale->value();
}

double DefaultPrintOptionsWidget::posterOverlap() const
{
    return m_posterOverlap->value();
}

bool DefaultPrintOptionsWidget::posterCutMarks() const
{
    return m_posterCutMarks->isChecked();
}

bool DefaultPrintOptionsWidget::posterLabels() const
{
    return m_posterLabels->isChecked();
}

}
