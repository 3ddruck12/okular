/*
    SPDX-FileCopyrightText: 2026 Okular N-Up Printer
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef NUPPRINTER_H
#define NUPPRINTER_H

#include <QObject>
#include <QString>

namespace Okular {

class Document;

class NUpPrinter : public QObject
{
    Q_OBJECT
public:
    struct NUpOptions {
        int pagesPerSheet; // e.g., 2, 4, 6, 9, 16
        int pageOrder;     // 0: Horizontal, 1: HorizRev, 2: Vert, 3: VertRev
        bool drawBorder;
    };

    explicit NUpPrinter(QObject *parent = nullptr);
    ~NUpPrinter() override;

    /**
     * @brief Generates an N-Up PDF from the document.
     * 
     * @param doc The Okular document.
     * @param outPath The file path where the temporary PDF will be saved.
     * @param opts N-Up printing options.
     * @return true on success, false otherwise.
     */
    bool generateNUpPdf(Okular::Document *doc, const QString &outPath, const NUpOptions &opts);
};

}

#endif
