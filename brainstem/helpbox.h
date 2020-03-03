#ifndef HELPBOX_H
#define HELPBOX_H

/* Copyright 2005-2020 Kendall F. Morris

This file is part of the USF Brainstem Data Visualization suite.

    The Brainstem Data Visualiation suite is free software: you can
    redistribute it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    The suite is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the suite.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <QDialog>

namespace Ui {
class HelpBox;
}

class HelpBox : public QDialog
{
    Q_OBJECT

public:
    explicit HelpBox(QWidget *parent = 0);
    ~HelpBox();

//private slots:
//    void on_buttonBox_accepted();

private slots:
    void on_helpText_4_anchorClicked(const QUrl &arg1);

    void on_helpText_5_anchorClicked(const QUrl &arg1);

private:
    Ui::HelpBox *ui;
};

#endif // HELPBOX_H
