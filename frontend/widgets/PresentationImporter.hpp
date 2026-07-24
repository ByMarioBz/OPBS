/******************************************************************************
    Copyright (C) 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
******************************************************************************/

#pragma once

#include <QString>
#include <QStringList>

struct PresentationImportResult {
	bool success = false;
	QStringList slidePaths;
	QString error;
};

class PresentationImporter {
public:
	static PresentationImportResult ImportPdf(const QString &sourcePath, const QString &destinationDirectory);
	static PresentationImportResult ImportPowerPoint(const QString &sourcePath,
							 const QString &destinationDirectory);
};
