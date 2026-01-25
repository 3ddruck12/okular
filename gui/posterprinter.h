/*
    SPDX-FileCopyrightText: 2026 Okular Poster Printer
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef POSTERPRINTER_H
#define POSTERPRINTER_H

#include <QObject>
#include <QString>

namespace Okular {

class Document;

class PosterPrinter : public QObject
{
    Q_OBJECT
public:
    struct PosterOptions {
        double tileScale;    // in percent, 100.0 = original size
        double overlap;      // in mm
        bool cutMarks;
        bool labels;
    };

    explicit PosterPrinter(QObject *parent = nullptr);
    ~PosterPrinter() override;

    /**
     * @brief Generates a tiled poster PDF from the document.
     * 
     * @param doc The Okular document.
     * @param outPath The file path where the temporary PDF will be saved.
     * @param opts Poster printing options.
     * @return true on success, false otherwise.
     */
    bool generatePosterPdf(Okular::Document *doc, const QString &outPath, const PosterOptions &opts);
};

}

#endif
