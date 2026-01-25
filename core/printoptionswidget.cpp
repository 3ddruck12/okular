/*
    SPDX-FileCopyrightText: 2019 Michael Weghorn <m.weghorn@posteo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "printoptionswidget.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QStackedWidget>
#include <QLabel>

#include <KLocalizedString>

namespace Okular
{
DefaultPrintOptionsWidget::DefaultPrintOptionsWidget(QWidget *parent)
    : PrintOptionsWidget(parent)
{
    setWindowTitle(i18n("Print Options"));
    QFormLayout *layout = new QFormLayout(this);
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
    
    connect(m_printModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DefaultPrintOptionsWidget::slotPrintModeChanged);
}

void DefaultPrintOptionsWidget::slotPrintModeChanged(int index)
{
    // Map combo index to stack index
    // Combo: 0 (FitArea), 1 (FitPage), 2 (Poster), 3 (Multiple)
    // Stack: 0 (Empty), 0 (Empty), 2 (Poster), 3 (Multiple) -> Wait, we added 2 empty widgets first?
    // Stack Index 0: Empty (for FitArea)
    // Stack Index 1: Empty (for FitPage)
    // Stack Index 2: Poster
    // Stack Index 3: N-Up
    
    // So index maps 1:1
    if (index >= 0 && index < m_modeStack->count()) {
        m_modeStack->setCurrentIndex(index);
    }
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
