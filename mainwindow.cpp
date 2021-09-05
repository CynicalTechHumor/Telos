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

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Initialize Ui
    ui->setupUi(this);

    // Initialize data fields
    active_task_list_         = nullptr;
    active_task_              = nullptr;
    active_task_saved_prereq_ = Task::PtrVector();
    active_filter_            = TaskFilter::kCurrent;
    active_sort_              = TaskSort::kName;
    drag_position_            = QPoint();
    prereq_combo_box_         = std::make_unique<QStringListModel>();
    depend_combo_box_         = std::make_unique<QStringListModel>();
    list_changed_             = false;
    task_list_dir_            = QDir(QDir::homePath());
    debug_mode_               = false;

    // Make a task list directory if one doesn't exist, and sets filters/sorting
    task_list_dir_.mkdir          ("Telos");
    task_list_dir_.cd             ("Telos");
    task_list_dir_.setFilter      (QDir::Files | QDir::Readable | QDir::Writable);
    task_list_dir_.setSorting     (QDir::Name);
    task_list_dir_.setNameFilters (QStringList("*.dat"));

    // Sets combo boxes to be edited by the QStringListModels
    ui->comboPrerequisites->setModel(prereq_combo_box_.get());
    ui->comboDependencies-> setModel(depend_combo_box_.get());

    // Load all task lists found in default directory
    QStringList task_list_names = task_list_dir_.entryList();
    for (int i=0; i<task_list_names.size(); ++i)
        LoadTaskListFromFile(task_list_names[i]);

    // Initalize most displayed fields
    UpdateDisplayOpenTaskLists();

    // Initialize remaining fields (those not affected by UpdateDisplayOpenTaskLists())
    ui->lwOpenTaskLists->setVisible(true );
    ui->teStatusBar->    setEnabled(false);

    // Hide the ugly Windows bar, and allow repositioning by dragging the menu bar
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    ui->menubar->installEventFilter(this);

    // Connect status signal
    connect(this, &MainWindow::SignalStatus, this, &MainWindow::SlotStatus);
}

MainWindow::~MainWindow()
{
    delete ui;
}

TaskList* MainWindow::GetOpenTaskListPtr(QString i_name)
{
    for (int i=0; i<open_task_lists_.size(); ++i)
        if (open_task_lists_[i]->GetTaskListName() == i_name) return open_task_lists_[i].get();
    return nullptr;
}

Task* MainWindow::GetSelectedTask(void)
{
    QList<QListWidgetItem *> selected_tasks = ui->lwTaskList->selectedItems();
    return (!active_task_list_ || selected_tasks.empty()) ?
           nullptr : active_task_list_->GetPtrFromTaskList(selected_tasks.first()->text());
}

void MainWindow::SelectPrereqToChange(TaskSelection i_select)
{
    // If no active task or list, throw logic exception
    if (!active_task_)      throw std::logic_error("ChangedPrereq failed: no active task");
    if (!active_task_list_) throw std::logic_error("ChangedPrereq failed: no active task list");

    // If adding prerequisite(s)...
    QStringList eligible;
    if(i_select == TaskSelection::kAddPrerequisite)
    {
        QStringList prereq = prereq_combo_box_->stringList(),
                    prereq_chain,
                    depend_chain,
                    completed;

        // ...get prerequisite chain, dependent chain, and completed tasks...
        for (int i=0; i<prereq.size(); ++i)
            active_task_list_->GetChainedPrereq(&prereq_chain, prereq[i]);
        active_task_list_->GetChainedDepend(&depend_chain, active_task_->GetTaskName());
        active_task_list_->GetCompleted    (&completed);

        // ...then remove them from the list of all tasks to leave only eligible ones
        eligible = active_task_list_->GetAllTaskNamesFromList();
        eligible = Task::SubtractTaskNames(eligible, prereq_chain);
        eligible = Task::SubtractTaskNames(eligible, depend_chain);
        eligible = Task::SubtractTaskNames(eligible, completed);
    }
    // If removing prerequisite(s), get current unchained prerequisites
    else if (i_select == TaskSelection::kRemovePrerequisite)
        eligible = prereq_combo_box_->stringList();

    // Sort the eligible list by name
    std::sort(eligible.begin(), eligible.end(), [](QString left, QString right) {return left < right;});

    // Open dialog box to select prerequisite(s) from the eligible tasks
    DialogTaskSelect* select_prerequisite = new DialogTaskSelect(this, i_select);
    select_prerequisite->UpdateTaskList(eligible);
    select_prerequisite->exec();
}

void MainWindow::ChangePrereq(QStringList i_list, TaskSelection i_select)
{
    // If no active task or list, throw logic exception
    if (!active_task_)      throw std::logic_error("ChangedPrereq failed: no active task");
    if (!active_task_list_) throw std::logic_error("ChangedPrereq failed: no active task list");

    // If adding preprequisites, iterate through incoming prerequisite chains
    // If a current prerequisite matches a chained prerequisite, remove the current one.
    // (The current one is redundant in this situation)
    QString status;
    if (i_select == TaskSelection::kAddPrerequisite)
    {
        QStringList prereqs_of_prereqs;
        for (int i=0; i<i_list.size(); i++)
        {
            active_task_list_->GetChainedPrereq(&prereqs_of_prereqs, i_list[i]);
            prereqs_of_prereqs = Task::SubtractTaskNames(prereqs_of_prereqs, i_list);
        }
        prereq_combo_box_->setStringList(Task::SubtractTaskNames((prereq_combo_box_->stringList() + i_list), prereqs_of_prereqs));
        status = "Added selected prerequisites to task \"" + active_task_->GetTaskName() + "\"";
    }
    // If removing prerequisites, remove input prerequisites from current prerequisites
    else if (i_select == TaskSelection::kRemovePrerequisite)
    {
        prereq_combo_box_->setStringList(Task::SubtractTaskNames(prereq_combo_box_->stringList(), i_list));
        status = "Removed selected prerequisites from task \"" + active_task_->GetTaskName() + "\"";
    }

    // Enable save if there were changes to prerequisites
    if(!i_list.empty()) ui->pbSaveChanges->setEnabled(true);
    emit SignalStatus(QtInfoMsg, status);
}

void MainWindow::SaveActiveTask(void)
{
    // Immediately return if there is no active task or task list
    if (!active_task_list_ || !active_task_)
        return;

    // Check the current name for duplicates in the task list
    // If duplicate, add "2" (or "3", or "4", or...) to the end
    QString current_name = ui->teTitle->toPlainText();
    if (active_task_list_->CheckDuplicateTaskName(current_name, active_task_))
    {
        unsigned int i = 2;
        while(active_task_list_->CheckDuplicateTaskName(current_name + " " + QString::number(i), active_task_))
            ++i;
        current_name = current_name + " " + QString::number(i);
    }

    // Save contents of name and description to their respective fields
    SetActiveTaskName        (current_name                     );
    SetActiveTaskDescription (ui->teDescription->toPlainText() );
    SetActiveTaskDeadline    (ui->cbDeadline   ->isChecked(),  ui->dtDeadline ->dateTime() );
    SetActiveTaskCompleted   (ui->cbCompleted  ->isChecked(),  ui->dtCompleted->dateTime() );

    // Assemble lists of previous, current, added, and removed prerequisites
    QStringList previous_prereq = Task::GetTaskNames(active_task_saved_prereq_);
    QStringList current_prereq  = prereq_combo_box_->stringList();
    QStringList added_prereq    = Task::SubtractTaskNames(current_prereq, previous_prereq);
    QStringList removed_prereq  = Task::SubtractTaskNames(previous_prereq, current_prereq);

    // Add active task ptr to dependencies of added prerequisite tasks
    // Remove active task ptr from dependencies of removed prerequisite tasks
    for(int i=0; i<added_prereq.size(); ++i)
        active_task_list_->GetPtrFromTaskList(added_prereq[i])->AddTaskDepend(active_task_);
    for(int i=0; i<removed_prereq.size(); ++i)
        active_task_list_->GetPtrFromTaskList(removed_prereq[i])->RemoveTaskDepend(active_task_);

    // Save current list of prerequisites to the active task, and flag list change
    active_task_->SetTaskPrereq(active_task_list_->GetPtrsFromTaskList(current_prereq));
    list_changed_ = true;
    QString status = "Saved changes to task \"" + active_task_->GetTaskName() + "\".";
    emit SignalStatus(QtInfoMsg, status);
}

void MainWindow::CreateTask(void)
{
    // Verify active task list
    if (!active_task_list_) throw std::logic_error("ChangedPrereq failed: no active task");

    // Assemble unique task name, adding numbers to the default name to prevent duplicate names
    QString unique_task_name = "Task ";
    int i = 1;
    while(active_task_list_->GetPtrFromTaskList(unique_task_name + QString::number(i)))
        i++;

    // Create a task with the assembled name to the active task list, and flag list change
    active_task_ = active_task_list_->AddTaskToList(unique_task_name + QString::number(i));
    list_changed_ = true;
    QString status = "Created new task \"" + active_task_->GetTaskName() + "\".";
    emit SignalStatus(QtInfoMsg, status);
}

void MainWindow::RemoveTask(void)
{
    // Throw exceptions if no task/list is active
    if (!active_task_)      throw std::logic_error("RemoveTask failed, no task is active");
    if (!active_task_list_) throw std::logic_error("RemoveTask failed, no task list is active");

    // Remove the active task from the list
    QString task_name = active_task_->GetTaskName();
    active_task_list_->RemoveTaskFromList(active_task_);

    // Set the active task ptr to null and flag list change
    active_task_ = nullptr;
    list_changed_ = true;

    QString status = "Removed task \"" + task_name + "\" from list.";
    emit SignalStatus(QtInfoMsg, status);
}

void MainWindow::CreateTaskList(void)
{
    // Get list name from an input dialog
    bool status_flag;
    QString list_name = QInputDialog::getText(this,
                                              tr("Create List"),
                                              tr("Enter task list name:"),
                                              QLineEdit::Normal,
                                              QString("My List"),
                                              &status_flag,
                                              Qt::Popup);

    // Return immediately if any invalid condition exists, with appropriate status message
    if(!status_flag)
    {
        emit SignalStatus(QtWarningMsg, "No task list created: dialog was closed without input.");
        return;
    }
    else if (list_name.isEmpty() || !IsValidTaskListTitle(list_name))
    {
        emit SignalStatus(QtWarningMsg, "No task list created: invalid input for list name");
        return;
    }
    else if (IsDuplicateTaskListTitle(list_name))
    {
        emit SignalStatus(QtWarningMsg, "No task list created: input list name already exists");
        return;
    }

    // ...otherwise, create list with the input name, add it to the open lists, and save to disk
    open_task_lists_.push_back(std::make_unique<TaskList>(list_name));
    SaveTaskListToFile(GetOpenTaskListPtr(list_name), TaskListSave::kNew);
    emit SignalStatus(QtInfoMsg, QString("Created new task list \"") + list_name + "\"");
}

bool MainWindow::RemoveTaskList(TaskList*& i_list)
{
    // Do nothing if the input list is a null ptr
    if (!i_list) return false;

    // Offer the user a last-chance prompt to cancel removal
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Remove list?",
                                                              "Delete the current list \"" + i_list->GetTaskListName() + "\"?",
                                                              QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No) return false;

    // Assemble the file name (list name with underscores instead of spaces)
    QString removed_file_name = i_list->GetTaskListName();
    while (removed_file_name.indexOf(' ') != -1)
        removed_file_name.replace(removed_file_name.indexOf(' '), 1, '_');

    // Find the input list in the open lists, erase it, and set the input list pointer to null
    // Throw logic exception if the input list wasn't among the open files
    bool found = false;
    for (int i=0; i<open_task_lists_.size() && !found; i++)
    {
        if (open_task_lists_[i].get() == i_list)
        {
            open_task_lists_.erase(open_task_lists_.begin() + i);
            found = true;
        }
    }
    if (!found)
        throw std::logic_error("Invalid input to RemoveTaskList(), input list not found in open task lists");
    i_list = nullptr;

    // Remove the file, return false if it can not be removed
    // Otherwise return true
    QFile removed_file((task_list_dir_.path() + "\\" + removed_file_name + ".dat").toLocal8Bit());
    if (!removed_file.remove())
    {
        QString status = "Failed to remove task list " + removed_file_name + " from disk.";
        emit SignalStatus(QtWarningMsg, status);
        return false;
    }

    QString status = "Removed task list \"" + removed_file_name + "\" from disk.";
    emit SignalStatus(QtInfoMsg, status);
    return true;
}

bool MainWindow::SaveTaskListToFile(TaskList* i_list, TaskListSave i_save_type)
{
    // If input list is a nullptr, do nothing
    if (!i_list) return false;

    // Determine the correct extension
    QString file_ext;
    if (i_save_type == TaskListSave::kCSV
     || i_save_type == TaskListSave::kCompleted)
        file_ext = ".csv";
    else
        file_ext = ".dat";

    // If exporting, opens a dialog for save file name/location...
    // Exit without saving if cancel is clicked, if nothing input, or if save directory is the reserved program one
    QString stored_name = i_list->GetTaskListName(),
            save_name;
    bool flag_name_changed = false;
    if (i_save_type == TaskListSave::kExport
     || i_save_type == TaskListSave::kCSV
     || i_save_type == TaskListSave::kCompleted )
    {
        QFileDialog dialog_save(nullptr);
        dialog_save.setFileMode  ( QFileDialog::AnyFile     );
        dialog_save.setViewMode  ( QFileDialog::Detail      );
        dialog_save.setAcceptMode( QFileDialog::AcceptSave  );
        dialog_save.setDirectory ( QDir::homePath()         );
        if (!dialog_save.exec())
        {
            QString status = "Dialog exited: save aborted.";
            emit SignalStatus(QtWarningMsg, status);
            return false;
        }
        QStringList selections = dialog_save.selectedFiles();
        if (selections.empty())
        {
            QString status = "No file selected: save aborted.";
            emit SignalStatus(QtWarningMsg, status);
            return false;
        }

        save_name = selections.first();
        QString save_dir = save_name;
        save_dir.truncate(save_name.lastIndexOf("/"));
        if (task_list_dir_.path() == save_dir)
        {
            QString status = "Can't save to Telos reserved directory: save aborted.";
            emit SignalStatus(QtWarningMsg, status);
            return false;
        }

        if(save_name.last(4) != file_ext)
            save_name.append(file_ext);

    }
    // If saving existing, use the current input task list name to determine the file name
    else if (i_save_type == TaskListSave::kActive)
    {
        // Validates current save name, exit without saving if invalid
        if (!IsValidTaskListTitle())
        {
            QString status = "Can not save list, input list name is invalid.";
            emit SignalStatus(QtWarningMsg, status);
            return false;
        }
        save_name = ui->teTitleTaskList->toPlainText();
        if (stored_name != save_name)
        {
            i_list->SetTaskListName(save_name);
            flag_name_changed = true;
        }
        MainWindow::ConvertSpaceToUnderscore(save_name);
        save_name.prepend(task_list_dir_.path() + "\\");
        save_name.append(file_ext);
    }
    // If saving new, make the file in the reserved Telos space
    else if (i_save_type == TaskListSave::kNew)
    {
        save_name = stored_name;
        MainWindow::ConvertSpaceToUnderscore(save_name);
        save_name.prepend(task_list_dir_.path() + "\\");
        save_name.append(file_ext);
    }

    // Opens save file - exit without saving if file cannot be opened
    QFile save_file(save_name.toLocal8Bit());
    if (!save_file.open(QIODevice::WriteOnly))
    {
        QString status = "Failed to open save file \"" + save_name + "\": save aborted.";
        emit SignalStatus(QtWarningMsg, status);
        return false;
    }

    // ****************************************************************************************
    // Save file (with the appropriate name) should have been successfully opened by this point
    // Begin assembling data from list to file
    // ****************************************************************************************

    // Construct default delineators for Telos .dat file
    QByteArray my_line_begin,
               my_divide_task,
               my_divide_field,
               my_divide_subfield,
               my_line_end;

    // If .dat file, use the Telos-defined delineators. No begin/end line necessary
    if (file_ext == ".dat")
    {
        my_divide_task.    append(MainWindow::DIVIDE_TASK    );
        my_divide_field.   append(MainWindow::DIVIDE_FIELD   );
        my_divide_subfield.append(MainWindow::DIVIDE_SUBFIELD);
    }
    // If .csv file, prompt the user for the desired field delineation (comma, tab, colon)
    // Add additional formatting to put every entry in quotes, and new lines for task delineation
    else
    {
        bool ok;
        QStringList delineation_options;
        delineation_options << tr("Comma") << tr("Tab")<< tr("Colon");
        QString select_delineate = QInputDialog::getItem(this,
                                                         tr("Select delineation character"),
                                                         tr("Select delineation character for CSV export"),
                                                         delineation_options,
                                                         0,
                                                         false,
                                                         &ok);
        if (ok && !select_delineate.isEmpty())
        {
            if     (select_delineate=="Comma") my_divide_field.append(',');
            else if(select_delineate=="Tab")   my_divide_field.append('\t');
            else if(select_delineate=="Colon") my_divide_field.append(':');
            else                               my_divide_field.append(',');
        }

        // New line to delineate entries (typical CSV format)
        my_divide_task.append('\n');

        // Add quotes at beginning of line, end of line, and before/after the field divide
        // Assures that every entry is within quotation marks (typical CSV format)
        my_line_begin.append('\"');
        my_divide_field.prepend('\"');
        my_divide_field.append('\"');
        my_line_end.append('\"');

        // Uses a simple comma and space for subfield divides
        my_divide_subfield.append(',');
        my_divide_subfield.append(' ');
    }

    // If exporting completed tasks, get the completed list; otherwise, get all the tasks
    Task::PtrVector save_list;
    if (active_task_list_)
            save_list = (i_save_type == TaskListSave::kCompleted) ? active_task_list_->GetAllCompleted() : active_task_list_->GetAllTaskPtrsFromList();

    // Construct a byte array with all information stored in list by iterating through each task
    // First entry is the list name: skip if listing completed tasks
    QByteArray data;
    if (i_save_type != TaskListSave::kCompleted)
    {
        data.append(my_line_begin);
        data.append(i_list->GetTaskListName().toLocal8Bit());
        data.append(my_line_end);
    }

    // All subsequent entries are individual tasks (deliminated by DIVIDE_TASK)
    for(Task* i : save_list)
    {
        // Start new task
        data.append(my_divide_task);
        data.append(my_line_begin);

        // Task Name
        QString my_name = i->GetTaskName();
        if (file_ext == ".csv") MainWindow::ConvertToDoubleQuotes(my_name);
        data.append(my_name.toLocal8Bit());
        data.append(my_divide_field);

        // Task Description: EMPTY for no description
        QString my_descript = i->GetTaskDescription();
        if (!my_descript.isEmpty())
        {
            if (file_ext == ".csv") MainWindow::ConvertToDoubleQuotes(my_descript);
            data.append(my_descript.toLocal8Bit());
        }
        else data.append(MainWindow::EMPTY);
        data.append(my_divide_field);

        // Task Deadline: EMPTY for no deadline
        if (i->GetTaskDeadline().isValid()) data.append(i->GetTaskDeadline().toString().toLocal8Bit());
        else data.append(MainWindow::EMPTY);
        data.append(my_divide_field);

        // Task Completed: EMPTY for not complete
        if (i->GetTaskCompleted().isValid()) data.append(i->GetTaskCompleted().toString().toLocal8Bit());
        else data.append(MainWindow::EMPTY);
        data.append(my_divide_field);

        // Task Prerequisites: EMPTY if no prerequisites, otherwise prerequisites seperated by DIVIDE_SUBFIELD
        Task::PtrVector prereq = i->GetTaskPrereq();
        if (!prereq.empty())
            for (int j=0; j<prereq.size(); ++j)
            {
                QString my_prereq = prereq[j]->GetTaskName();
                if (file_ext == ".csv") MainWindow::ConvertToDoubleQuotes(my_prereq);
                data.append(my_prereq.toLocal8Bit());
                if (j != prereq.size()-1) data.append(my_divide_subfield);
            }
        else data.append(MainWindow::EMPTY);
        data.append(my_divide_field);

        // Task Dependencies: EMPTY only if no dependencies, otherwise dependencies seperated by DIVIDE_SUBFIELD
        Task::PtrVector depend = i->GetTaskDepend();
        if (!depend.empty())
            for (int j=0; j<depend.size(); ++j)
            {
                QString my_depend = depend[j]->GetTaskName();
                if (file_ext == ".csv") MainWindow::ConvertToDoubleQuotes(my_depend);
                data.append(my_depend.toLocal8Bit());
                if (j != depend.size()-1) data.append(my_divide_subfield);
            }
        else data.append(MainWindow::EMPTY);
        data.append(my_line_end);
    }

    // Write the data to the save file, close it, and reset the "list changed" flag
    save_file.write(data);
    save_file.close();
    list_changed_=false;

    // If saving active & name changed, remove the previous save file
    if (flag_name_changed)
    {
        MainWindow::ConvertSpaceToUnderscore(stored_name);
        QFile previous_file((task_list_dir_.path() + "\\" + stored_name + ".dat").toLocal8Bit());
        if (!previous_file.remove())
        {
            QString status = "Failed to remove \"" + stored_name + "\" from disk.";
            emit SignalStatus(QtWarningMsg, status);
        }
    }

    // Return true to indicate a successful write
    QString status = QString("Successfully ") + (i_save_type == TaskListSave::kExport ? "exported" : "saved") + " task list \"" + i_list->GetTaskListName() + "\" to disk.";
    emit SignalStatus(QtInfoMsg, status);
    return true;
}

bool MainWindow::LoadTaskListFromFile(QString i_file_name)
{
    // If no file name provided, prompt user for file name/location; if cancel is clicked, exit without saving
    QString load_name;
    if (i_file_name.isEmpty())
    {
        QFileDialog dialog_load(nullptr);
        dialog_load.setFileMode(QFileDialog::ExistingFile);
        dialog_load.setNameFilter(tr("Data Files (*.dat)"));
        dialog_load.setViewMode(QFileDialog::Detail);
        dialog_load.setAcceptMode(QFileDialog::AcceptOpen);
        dialog_load.setDirectory(QDir::homePath());
        if (!dialog_load.exec())
        {
            emit SignalStatus(QtWarningMsg, "Dialog exited: load aborted.");
            return false;
        }
        QStringList selections = dialog_load.selectedFiles();
        if (selections.empty())
        {
            emit SignalStatus(QtWarningMsg, "Load aborted: No file selected: ");
            return false;
        }
        load_name = selections.first();
    }
    else
        load_name = task_list_dir_.path() + "\\" + i_file_name;

    // Opens, reads, and closes selected file; if file cannot be opened, exit function without loading
    // Splits file into a list of byte arrays, delineated by DIVIDE_TASK
    QFile load_file(load_name);
    if (!load_file.open(QIODevice::ReadOnly))
    {
        emit SignalStatus(QtWarningMsg, "Failed to open task list \"" + load_name + "\" from disk.");
        return false;
    }
    QList<QByteArray> data_file = load_file.readAll().split(MainWindow::DIVIDE_TASK);
    load_file.close();

    // First item is incoming list name; if name is already in open lists, exit immediately
    // Otherwise, create task list with that name; if name is already in list, exit immediately
    QString list_name = QString::fromUtf8(data_file.first().toStdString());
    data_file.pop_front();
    if (IsDuplicateTaskListTitle(list_name))
    {
        emit SignalStatus(QtWarningMsg, "Load aborted: File name already exists");
        return false;
    }
    open_task_lists_.push_back(std::make_unique<TaskList>(list_name));
    TaskList* o_list = open_task_lists_.back().get();

    // *******************************************************************************************
    // Task list with valid name should be successfully opened by this point
    // Going forward, invalid inputs will result in logged errors, with unrecoverable data ignored
    // *******************************************************************************************

    // Persistent containers used to capture the incoming list of prereqs/depends for each task
    // Memory addresses have not yet been assigned; all tasks must be in memory before assigning prereq/depend pointers
    std::vector<QString>     i_names_all;
    std::vector<QStringList> i_prereq_all, i_depend_all;

    // Each remaining item is a task entry
    // Iterate through each item, reading task information from each one
    for (int i=0; i<data_file.size(); ++i)
    {
        // Temporary containers used to capture incoming task information used in task constructor
        // Released/reset with each iteration
        uint8_t     i_byte;
        QString     i_name,     i_description;
        QDateTime   i_deadline, i_completed;
        QStringList i_prereq,   i_depend;

        // Split current task entry into constituent fields
        QList<QByteArray> data_task = data_file[i].split(MainWindow::DIVIDE_FIELD);

        // Task name: Add to a container for later use in assigning prerequisites
        i_name = QString::fromUtf8(data_task[0].toStdString());
        i_names_all.push_back(i_name);

        // Task description: EMPTY byte indicates no description
        i_byte = data_task[1].at(0);
        if (i_byte == MainWindow::EMPTY) i_description = "";
        else i_description = QString::fromUtf8(data_task[1].toStdString());

        // Task deadline: EMPTY byte indicates no deadline
        i_byte = data_task[2].at(0);
        if (i_byte == MainWindow::EMPTY) i_deadline = QDateTime();
        else i_deadline = QDateTime::fromString(QString::fromUtf8(data_task[2].toStdString()));

        // Task completed: EMPTY byte indicates not completed
        i_byte = data_task[3].at(0);
        if (i_byte == MainWindow::EMPTY) i_completed = QDateTime();
        else i_completed = QDateTime::fromString(QString::fromUtf8(data_task[3].toStdString()));

        // Task prerequisites: EMPTY byte indicates no prerequisites
        // Divide prerequsites by DIVIDE_SUBFIELD, push each task prerequisite into a string list
        // Push final string list into a list of prerequisites for all tasks, to be turned into pointers later
        i_byte = data_task[4].at(0);
        if (i_byte != MainWindow::EMPTY)
        {
            QList<QByteArray> data_prereq = data_task[4].split(MainWindow::DIVIDE_SUBFIELD);
            for (int j=0; j<data_prereq.size(); ++j)
                i_prereq.push_back(QString::fromUtf8(data_prereq[j].toStdString()));
        }
        i_prereq_all.push_back(i_prereq);

        // Task dependents: EMPTY byte indicates no dependents
        // Divide dependents by DIVIDE_SUBFIELD, push each task dependent into a string list
        // Push final string list into a list of dependents for all tasks, to be turned into pointers later
        i_byte = data_task[5].at(0);
        if (i_byte != MainWindow::EMPTY)
        {
            QList<QByteArray> data_depend = data_task[5].split(MainWindow::DIVIDE_SUBFIELD);
            for (int j=0; j<data_depend.size(); ++j)
                i_depend.push_back(QString::fromUtf8(data_depend[j].toStdString()));
        }
        i_depend_all.push_back(i_depend);

        // Construct task object with collected information, and add to task list
        o_list->AddTaskToList(i_name, i_description, i_deadline, i_completed);
    }

    // Iterate through all tasks in list, adding prerequisites/dependents to each
    for(int i=0; i<i_names_all.size(); ++i)
    {
        o_list->SetTaskPrereqFromList(i_names_all[i], i_prereq_all[i]);
        o_list->SetTaskDependFromList(i_names_all[i], i_depend_all[i]);
    }

    // If file was imported, save to disk immediately
    if (i_file_name.isEmpty())
        SaveTaskListToFile(o_list, TaskListSave::kNew);

    // Return true to indicate successful load
    QString status = "Successfully loaded task list \"" + list_name + "\" from disk.";
    emit SignalStatus(QtInfoMsg, status);
    return true;
}

void MainWindow::ConvertToDoubleQuotes(QString &i_string)
{
    for (int i=0; i<i_string.size(); ++i)
        if (i_string[i] == '\"')
            i_string.insert(i++, '\"');
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != ui->menubar)
        return false;

    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
        if (mouse_event->button() == Qt::LeftButton) drag_position_ = mouse_event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    else if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
        if (mouse_event->buttons() & Qt::LeftButton) move(mouse_event->globalPosition().toPoint() - drag_position_);
    }
    return false;
}

void MainWindow::UpdateDisplayOpenTaskLists(void)
{
    // Set the active task list to the selected list
    QListWidgetItem* selected_item = ui->lwOpenTaskLists->currentItem();
    active_task_list_ = selected_item ? GetOpenTaskListPtr(selected_item->text()) : nullptr;

    // Get names of all open lists, then sorts the list
    QStringList open_task_list_names = QStringList();
    if (!open_task_lists_.empty())
        for(int i=0; i<open_task_lists_.size(); ++i)
            open_task_list_names.push_back(open_task_lists_[i]->GetTaskListName());
    std::sort(open_task_list_names.begin(), open_task_list_names.end(), [](QString left, QString right) {return left < right;});

    // Clear any displayed task lists, and then adds back the sorted open list names
    ui->lwOpenTaskLists->clear();
    ui->lwOpenTaskLists->addItems(open_task_list_names);

    // If a task list was previously active, re-select it if still in list
    // If it is no longer in the list, set active task list to null ptr
    if (active_task_list_)
    {
        QList<QListWidgetItem *> active_item = ui->lwOpenTaskLists->findItems(active_task_list_->GetTaskListName(), Qt::MatchExactly);
        if (!active_item.isEmpty())
            ui->lwOpenTaskLists->setCurrentItem(active_item.first());
        else
            active_task_list_ = nullptr;
    }

    // Updates the fields relating to the active task list
    UpdateDisplayActiveTaskList();
}

void MainWindow::UpdateDisplayActiveTaskList(void)
{
    // Display active task list title and enables/disables field
    UpdateDisplayText(active_task_list_, active_task_list_ ? active_task_list_->GetTaskListName() : "No task list selected", ui->teTitleTaskList);

    // Iterate through all tasks, and add individual tasks to filtered list per active filter
    QStringList filtered_tasks;
    if (active_task_list_)
    {
        for(int i=0; i<active_task_list_->GetTaskListSize(); ++i)
        {
            Task* my_task_ptr = (*active_task_list_)[i];
            if (active_filter_ == TaskFilter::kAll                                                                                        // TaskFilter::all       - Add all tasks to filtered list (so, y'know, don't filter it)
            || (active_filter_ == TaskFilter::kCompleted && my_task_ptr->IsTaskComplete())                                                    // TaskFilter::completed - Add task to filtered list if complete
            || (active_filter_ == TaskFilter::kCurrent   && !(my_task_ptr->IsTaskComplete()) && my_task_ptr->AreTaskPrereqComplete())      // TaskFilter::current   - Add task to filtered list if task is incomplete, but all prerequisites are complete
            || (active_filter_ == TaskFilter::kPending   && !(my_task_ptr->IsTaskComplete()) && !(my_task_ptr->AreTaskPrereqComplete())))  // TaskFilter::pending   - Add task to filtered list if task is incomplete, and any prerequisites are incomplete
                filtered_tasks.append(my_task_ptr->GetTaskName());
        }
    }

    // Create a stack of sorts to perform based on the active sort
    std::vector<TaskSort> sorting_stack;
    if (active_sort_ == TaskSort::kName)
    {
        sorting_stack.push_back(TaskSort::kName);
        sorting_stack.push_back(TaskSort::kDeadline);
    }
    else if (active_sort_ == TaskSort::kDeadline)
    {
        sorting_stack.push_back(TaskSort::kDeadline);
        sorting_stack.push_back(TaskSort::kName);
    }

    // Sort filtered tasks in stack order
    while (!sorting_stack.empty())
    {
        TaskSort current_sort = sorting_stack.back();
        sorting_stack.pop_back();
        if      (current_sort == TaskSort::kName)
            std::sort(filtered_tasks.begin(), filtered_tasks.end(), [](QString left, QString right) {return left < right;});
        else if (current_sort == TaskSort::kDeadline)
            std::sort(filtered_tasks.begin(), filtered_tasks.end(), [this](QString left, QString right) {return active_task_list_->GetPtrFromTaskList(left)->GetTaskDeadline() < active_task_list_->GetPtrFromTaskList(right)->GetTaskDeadline();});
    }

    // Clear the displayed task list, adds sorted and filtered tasks
    ui->lwTaskList->clear();
    ui->lwTaskList->addItems(filtered_tasks);

    // If a task was previously active, re-select it if still in list
    // If it is no longer in the list, set active task to null ptr
    if (active_task_)
    {
        QList<QListWidgetItem *> active_item = ui->lwTaskList->findItems(active_task_->GetTaskName(), Qt::MatchExactly);
        if (!active_item.empty())
            ui->lwTaskList->setCurrentItem(active_item.first());
        else
            active_task_ = nullptr;
    }

    // Enable task creation if a list is active
    ui->pbCreateTask->setEnabled(active_task_list_);

    // Update the active task information
    UpdateDisplayActiveTask();
}

void MainWindow::UpdateDisplayActiveTask(void)
{
    // Get the selected task (nullptr if nothing selected)
    active_task_ = GetSelectedTask();

    // Update all fields with current task's information
    UpdateDisplayText          (active_task_, GetActiveTaskName(),        ui->teTitle                                     );
    UpdateDisplayText          (active_task_, GetActiveTaskDescription(), ui->teDescription                               );
    UpdateDisplayCombo         (active_task_, GetActiveTaskPrereqSaved(), ui->comboPrerequisites, prereq_combo_box_.get() );
    UpdateDisplayCombo         (active_task_, GetActiveTaskDependSaved(), ui->comboDependencies,  depend_combo_box_.get() );
    UpdateDisplayDateTimeSaved (active_task_, GetActiveTaskDeadline(),    ui->cbDeadline,         ui->dtDeadline          );
    UpdateDisplayDateTimeSaved (active_task_, GetActiveTaskCompleted(),   ui->cbCompleted,        ui->dtCompleted         );

    // Record the saved list of prerequisites when a task is loaded
    // Used to find changes when saving updated information
    active_task_saved_prereq_ = active_task_ ? GetActiveTaskPrereqSaved() : Task::PtrVector();

    // Enable/disable task controls for saving, deleting, and linking/unlinking prerequisites
    ui->pbSaveChanges        ->setEnabled(false);
    ui->pbRemoveTask         ->setEnabled(active_task_);
    ui->pbLinkPrerequisite   ->setEnabled(active_task_);
    ui->pbUnlinkPrerequisite ->setEnabled(active_task_);
}

void MainWindow::UpdateDisplayText(bool            i_enable,
                                   QString         i_text,
                                   QPlainTextEdit* i_text_edit)
{
    // Enable/disable the field and sets contents to input value
    i_text_edit->setEnabled(i_enable);
    i_text_edit->setPlainText(i_text);
}

void MainWindow::UpdateDisplayCombo(bool               i_enable,
                                    Task::PtrVector    i_list,
                                    QComboBox*         i_combo_box,
                                    QStringListModel*  i_model)
{
    // Enable/disable the field
    i_combo_box->setEnabled(i_enable);

    // Create a string list of all input task names, then set the combo model with them
    QStringList updated_list;
    for (Task *i : i_list) updated_list.push_back(i->GetTaskName());
    i_model->setStringList(updated_list);
}

void MainWindow::UpdateDisplayDateTimeSaved(bool           i_enable,
                                            QDateTime      i_date_time,
                                            QCheckBox*     i_check_box,
                                            QDateTimeEdit* i_date_time_edit)
{
    // Enable/disable the check box associated with the DateTime field
    i_check_box->setEnabled(i_enable);

    // If check box is disabled or the input DateTime is invalid, hide DateTime field and uncheck box
    // Otherwise, show DateTime field, set to input value, and check the associated check box
    if (!i_enable || !i_date_time.isValid())
    {
        i_date_time_edit->hide();
        i_check_box->setChecked(false);
    }
    else
    {
        i_date_time_edit->show();
        i_date_time_edit->setDateTime(i_date_time);
        i_check_box->setChecked(true);
    }
}

void MainWindow::UpdateDisplayDateTimeCurrent(QDateTime      i_time,
                                              QCheckBox*     i_check_box,
                                              QDateTimeEdit* i_date_time_edit)
{
    // Show/hide the DateTime field in accordance with associated checkbox
    // Initialize it to the current DateTime if one isn't already saved
    if (i_check_box->isChecked())
    {
        i_date_time_edit->show();
        i_date_time_edit->setDateTime(i_time.isValid() ? i_time : QDateTime::currentDateTime());
    }
    else i_date_time_edit->hide();
}

bool MainWindow::IsValidTaskListTitle(QString i_name)
{
    // Task list names must contain only a-z, A-Z, 0-9, spaces, and dashes
    // The first character cannot be a space or a dash
    QRegularExpression valid_characters("^\\w[\\w -]+$");

    // If input string is empty, check the user input name for validity
    // Otherwise, check the input string
    QString checked_name;
    if (i_name.isEmpty())
        checked_name = ui->teTitleTaskList->toPlainText();
    else
        checked_name = i_name;
    QRegularExpressionMatch matched_text = valid_characters.match(checked_name);

    // If checking user input, set input field to normal color if valid. Otherwise, set to red.
    if (i_name.isEmpty())
    {
        if (matched_text.hasMatch())
            ui->teTitleTaskList->setStyleSheet("background-color:rgb(64, 64, 64)");
        else
            ui->teTitleTaskList->setStyleSheet("background-color:rgb(64, 0, 0)");
    }

    // Return if valid
    return matched_text.hasMatch();
}

bool MainWindow::IsDuplicateTaskListTitle(QString i_name)
{
    // If no name was input, check the current user input name
    if (i_name.isEmpty())
        i_name = ui->teTitleTaskList->toPlainText();

    // Searches open lists for input name
    bool found = false;
    for (int i=0; i<open_task_lists_.size() && !found; ++i)
        if (open_task_lists_[i]->GetTaskListName() == i_name)
            found = true;

    // Return true if a duplicate name was found, otherwise false
    return found;
}

void MainWindow::PromptSaveTask(void)
{
    if (!ui->pbSaveChanges->isEnabled()) return;
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Save Task?",
                                                              "Save changes to the current task?",
                                                              QMessageBox::Save|QMessageBox::Discard);
    if (reply == QMessageBox::Save) SaveActiveTask();
}

void MainWindow::PromptSaveTaskList(void)
{
    if (!list_changed_) return;
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Save List?",
                                                              "Save changes to the current list?",
                                                              QMessageBox::Save|QMessageBox::Discard);
    if (reply == QMessageBox::Save) SaveTaskListToFile(active_task_list_, TaskListSave::kActive);
}

void MainWindow::SlotStatus(QtMsgType i_type, QString i_message)
{
    ui->teStatusBar->setPlainText(i_message);
    if (debug_mode_)
    {
        switch (i_type)
        {
        case QtDebugMsg:
            qDebug() << i_message;
            break;
        case QtInfoMsg:
            qInfo() << i_message;
            break;
        case QtWarningMsg:
            qWarning() << i_message;
            break;
        case QtCriticalMsg:
            qCritical() << i_message;
            break;
        }
    }
}

void MainWindow::on_actionTelos_triggered()
{
    QFile read_me_file(":/text/README.md");
    if (!read_me_file.open(QIODeviceBase::ReadOnly))
        return;
    QString read_me_string = QString::fromStdString(read_me_file.readAll().toStdString());
    read_me_file.close();

    QMessageBox about_telos_msg_box(ui->centralwidget);
    about_telos_msg_box.setTextFormat(Qt::RichText);   //this is what makes the links clickable
    about_telos_msg_box.setText(read_me_string);
    about_telos_msg_box.setWindowTitle("About Telos");
    about_telos_msg_box.setStandardButtons(QMessageBox::Ok);
    about_telos_msg_box.exec();
}


void MainWindow::on_actionClearCompleted_triggered()
{
    if (!active_task_list_) return;
    PromptSaveTask();
    PromptSaveTaskList();
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Export to CSV?",
                                                              "Export completed tasks to CSV before deleting them FOREVER?",
                                                              QMessageBox::Save|QMessageBox::Discard);
    if (reply == QMessageBox::Save) SaveTaskListToFile(active_task_list_, TaskListSave::kCompleted);
    active_task_list_->RemoveTasksFromList(active_task_list_->GetAllCompleted());
    UpdateDisplayActiveTaskList();
}
