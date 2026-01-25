/*
    SPDX-FileCopyrightText: 2019 Michael Weghorn <m.weghorn@posteo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PRINTOPTIONSWIDGET_H
#define PRINTOPTIONSWIDGET_H

#include <QWidget>

#include "okularcore_export.h"

class QComboBox;

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
class OKULARCORE_EXPORT DefaultPrintOptionsWidget : public PrintOptionsWidget
{
    Q_OBJECT

public:
    explicit DefaultPrintOptionsWidget(QWidget *parent = nullptr);

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
    class QStackedWidget *m_modeStack;
    
    // Poster widgets
    class QDoubleSpinBox *m_posterTileScale;
    class QDoubleSpinBox *m_posterOverlap;
    class QCheckBox *m_posterCutMarks;
    class QCheckBox *m_posterLabels;
    
    // N-Up widgets
    class QComboBox *m_nUpPagesPerSheet;
    class QComboBox *m_nUpPageOrder;
    class QCheckBox *m_nUpDrawBorder;

private Q_SLOTS:
    void slotPrintModeChanged(int index);
};

}

#endif
