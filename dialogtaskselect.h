//    This file is part of Telos
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

#ifndef DIALOGTASKSELECT_H
#define DIALOGTASKSELECT_H

#include "ui_dialogtaskselect.h"

// Add/remove selected prerequisites
enum class TaskSelection {kAddPrerequisite, kRemovePrerequisite};

namespace Ui {
class DialogTaskSelect;
}

class DialogTaskSelect : public QDialog
{
    Q_OBJECT

public:

    // Constructor & Destructor
    explicit DialogTaskSelect(QWidget* = nullptr, TaskSelection = TaskSelection::kAddPrerequisite);
    ~DialogTaskSelect();

    // Add selectable items to dialog
    void UpdateTaskList(QStringList i_string_list) { ui->lwDialogTaskList->addItems(i_string_list); }

private slots:

    // If accepted, signal the main window with the selected prerequisites
    // If rejected, close dialog without doing anything
    void on_buttonBox_accepted();
    void on_buttonBox_rejected() { this->close(); }

private:

    // Data
    Ui::DialogTaskSelect *ui;
    TaskSelection selection_type_;

signals:

    // Signal selection made by user
    void SignalChangePrerequisites(QStringList, TaskSelection);

};

#endif // DIALOGTASKSELECT_H
