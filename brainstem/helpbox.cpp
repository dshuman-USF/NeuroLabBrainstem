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

#include "helpbox.h"
#include "ui_helpbox.h"
#include <QDesktopServices>

HelpBox::HelpBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpBox)
{
    ui->setupUi(this);
}

HelpBox::~HelpBox()
{
    delete ui;
}

//void HelpBox::on_buttonBox_accepted()
//{
//   close();
//}

void HelpBox::on_helpText_4_anchorClicked(const QUrl &arg1)
{
   QDesktopServices::openUrl(QUrl(arg1));
}

void HelpBox::on_helpText_5_anchorClicked(const QUrl &arg1)
{
    on_helpText_4_anchorClicked(arg1);
}
