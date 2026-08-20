/******************************************************************************
    Copyright (C) 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
******************************************************************************/

#pragma once

#include <QString>

class QMainWindow;

namespace OpbsDesignSystem {

QString ApplicationStyleSheet();
QString PresenterStyleSheet();
void ApplyToMainWindow(QMainWindow *window);

} // namespace OpbsDesignSystem
