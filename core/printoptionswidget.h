/*
    SPDX-FileCopyrightText: 2019 Michael Weghorn <m.weghorn@posteo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PRINTOPTIONSWIDGET_H
#define PRINTOPTIONSWIDGET_H

#include <QWidget>

#include "okularcore_export.h"

class QComboBox;
class QStackedWidget;
class QDoubleSpinBox;
class QCheckBox;

namespace Okular
{
/**
 * @short Abstract base class for an extra print options widget in the print dialog.
 */
class OKULARCORE_EXPORT PrintOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PrintOptionsWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
    }
    enum PrintMode {
        NoScaling,
        FitToPrintableArea,
        FitToPage,
        Poster,
        MultiplePages
    };

    virtual PrintMode printMode() const { return NoScaling; }
    
    // Poster specific
    virtual double posterTileScale() const { return 100.0; }
    virtual double posterOverlap() const { return 0.0; }
    virtual bool posterCutMarks() const { return false; }
    virtual bool posterLabels() const { return false; }

    virtual bool ignorePrintMargins() const { return false; }

    // N-Up specific
    virtual int nUpPagesPerSheet() const { return 1; }
    virtual int nUpPageOrder() const { return 0; } // 0: Horizontal, 1: HorizRev, 2: Vert, 3: VertRev
    virtual bool nUpDrawBorder() const { return false; }
};

/**
 * @short The default okular extra print options widget.
 *
 * It just implements the required method 'ignorePrintMargins()' from
 * the base class 'PrintOptionsWidget'.
 */
class PosterPreviewWidget;
class Document;

/**
 * @short The default okular extra print options widget.
 *
 * It just implements the required method 'ignorePrintMargins()' from
 * the base class 'PrintOptionsWidget'.
 */
class OKULARCORE_EXPORT DefaultPrintOptionsWidget : public PrintOptionsWidget
{
    Q_OBJECT

public:
    explicit DefaultPrintOptionsWidget(QWidget *parent = nullptr, Okular::Document *doc = nullptr);

    bool ignorePrintMargins() const override;
    PrintOptionsWidget::PrintMode printMode() const override;

    // Poster specific
    double posterTileScale() const override;
    double posterOverlap() const override;
    bool posterCutMarks() const override;
    bool posterLabels() const override;
    
    // N-Up specific
    int nUpPagesPerSheet() const override;
    int nUpPageOrder() const override;
    bool nUpDrawBorder() const override;

private:
    QComboBox *m_printModeCombo;
    QStackedWidget *m_modeStack;
    
    // Poster widgets
    QDoubleSpinBox *m_posterTileScale;
    QDoubleSpinBox *m_posterOverlap;
    QCheckBox *m_posterCutMarks;
    QCheckBox *m_posterLabels;
    PosterPreviewWidget *m_posterPreview;
    
    // N-Up widgets
    QComboBox *m_nUpPagesPerSheet;
    QComboBox *m_nUpPageOrder;
    QCheckBox *m_nUpDrawBorder;

private Q_SLOTS:
    void slotPrintModeChanged(int index);
    void slotPosterOptionChanged();
};

}

#endif
