//    This file is part of Telos v0.9.1
//    Copyright (c) 2021, Cynical Tech Humor LLC

//    Telos is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    Telos is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with Telos.  If not, see <https://www.gnu.org/licenses/>.

//    Source code is available at:
//    <https://github.com/CynicalTechHumor/Telos>

#include "dialogtaskselect.h"

DialogTaskSelect::DialogTaskSelect(QWidget *parent, TaskSelection i_select) :
    QDialog(parent),
    ui(new Ui::DialogTaskSelect)
{
    ui->setupUi(this);
    selection_type_ = i_select;
    QObject::connect(this, SIGNAL(SignalChangePrerequisites(QStringList, TaskSelection)), parent, SLOT(SlotChangePrerequisites(QStringList, TaskSelection)), Qt::AutoConnection);
}

DialogTaskSelect::~DialogTaskSelect()
{
    delete ui;
}

void DialogTaskSelect::on_buttonBox_accepted(void)
{
    QStringList selected_items;
    for (QListWidgetItem *i : ui->lwDialogTaskList->selectedItems())
        selected_items.append(i->text());
    emit SignalChangePrerequisites(selected_items, selection_type_);
    this->close();
}
