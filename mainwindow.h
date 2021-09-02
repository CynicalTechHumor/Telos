//    Telos - Version 0.9.0
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "./ui_mainwindow.h"

#include "dialogtaskselect.h"
#include "task.h"

#include <QtGui>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

// Selected filter
enum class TaskFilter {kCurrent, kCompleted, kPending, kAll};

// Sorting options
enum class TaskSort {kName, kDeadline};

// Save options
enum class TaskListSave {kNew, kActive, kExport};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    // Constructor/Destructor
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static constexpr uint8_t EMPTY           = 0;
    static constexpr uint8_t DIVIDE_FIELD    = 1;
    static constexpr uint8_t DIVIDE_SUBFIELD = 2;
    static constexpr uint8_t DIVIDE_TASK     = 3;

private:

    // Data

    Ui::MainWindow*                        ui;
    std::vector<std::unique_ptr<TaskList>> open_task_lists_;
    TaskList*                              active_task_list_;
    Task*                                  active_task_;
    std::vector<Task*>                     active_task_saved_prereq_;
    TaskFilter                             active_filter_;
    TaskSort                               active_sort_;
    QPoint                                 drag_position_;
    std::unique_ptr<QStringListModel>      prereq_combo_box_;
    std::unique_ptr<QStringListModel>      depend_combo_box_;
    bool                                   list_changed_;
    QDir                                   task_list_dir_;
    bool                                   debug_mode_;

    // Accessors - Returns saved information for the selected task & task list
    // Returns empty QString/QDateTime/std::vector if no task/list is active

    QString            GetActiveTaskName        (void) { return active_task_      ? active_task_->      GetTaskName()        : QString();            }
    QString            GetActiveTaskDescription (void) { return active_task_      ? active_task_->      GetTaskDescription() : QString();            }
    QDateTime          GetActiveTaskDeadline    (void) { return active_task_      ? active_task_->      GetTaskDeadline()    : QDateTime();          }
    QDateTime          GetActiveTaskCompleted   (void) { return active_task_      ? active_task_->      GetTaskCompleted()   : QDateTime();          }
    std::vector<Task*> GetActiveTaskPrereqSaved (void) { return active_task_      ? active_task_->      GetTaskPrereq()      : std::vector<Task*>(); }
    std::vector<Task*> GetActiveTaskDependSaved (void) { return active_task_      ? active_task_->      GetTaskDepend()      : std::vector<Task*>(); }
    QString            GetActiveTaskListName    (void) { return active_task_list_ ? active_task_list_-> GetTaskListName()    : QString();            }

    //
    TaskList* GetOpenTaskListPtr(QString i_name);
    Task*     GetSelectedTask(void);

    // ********
    // Mutators
    // ********

    // Save information for the active task
    // Return false if no task is active, otherwise true
    void SetActiveTaskName        (QString i_name)                     { if (active_task_) active_task_->SetTaskName       (i_name);                             }
    void SetActiveTaskDescription (QString i_description)              { if (active_task_) active_task_->SetTaskDescription(i_description);                      }
    void SetActiveTaskDeadline    (bool i_flag, QDateTime i_date_time) { if (active_task_) active_task_->SetTaskDeadline   (i_flag ? i_date_time : QDateTime()); }
    void SetActiveTaskCompleted   (bool i_flag, QDateTime i_date_time) { if (active_task_) active_task_->SetTaskCompleted  (i_flag ? i_date_time : QDateTime()); }

    //
    void SelectPrereqToChange (TaskSelection);
    void ChangePrereq         (QStringList, TaskSelection);

    //
    void SaveActiveTask       (void);
    void CreateTask           (void);
    void RemoveTask           (void);

    //
    void CreateTaskList       (void);
    bool RemoveTaskList       (TaskList*&);
    bool SaveTaskListToFile   (TaskList*, TaskListSave);
    bool LoadTaskListFromFile (QString = QString());

    // ******
    // Static
    // ******

    static void ConvertSpaceToUnderscore(QString &i_name) { while (i_name.indexOf(' ') != -1) i_name.replace(i_name.indexOf(' '), 1, '_'); }

    // *********
    // Interface
    // *********

    // Move window if menu bar is clicked & dragged
    bool eventFilter (QObject*, QEvent*);

    // Update open task lists, active list, or active task
    // Each of these functions calls the one below it after finishing
    void UpdateDisplayOpenTaskLists  (void);
    void UpdateDisplayActiveTaskList (void);
    void UpdateDisplayActiveTask     (void);

    // Updates individual fields with input data
    void UpdateDisplayText            (bool,      QString,            QPlainTextEdit*                   );
    void UpdateDisplayCombo           (bool,      std::vector<Task*>, QComboBox*,     QStringListModel* );
    void UpdateDisplayDateTimeSaved   (bool,      QDateTime,          QCheckBox*,     QDateTimeEdit*    );
    void UpdateDisplayDateTimeCurrent (QDateTime, QCheckBox*,         QDateTimeEdit*                    );

    // ValidateTaskListTitle()
    // Empty input string checks the user input, otherwise checks validity of input string
    // When checking the user input, turns task list name field red when invalid data has been input
    bool ValidateTaskListTitle(QString = QString());

    // PromptSaveTask(), PromptSaveTaskList()
    // If there are unsaved changes to the task/list, prompts the user to save them
    // If no unsaved changes, does nothing
    void PromptSaveTask(void);
    void PromptSaveTaskList(void);

signals:

    void SignalStatusBarUpdate(QtMsgType, QString);

public slots:

    void SlotStatusBarUpdate(QtMsgType, QString);

private slots:

    void on_actionCreateList_triggered(void)
    {
        CreateTaskList();
        UpdateDisplayOpenTaskLists();
    }

    void on_actionSaveList_triggered(void)
    {
        SaveTaskListToFile(active_task_list_, TaskListSave::kActive);
        UpdateDisplayOpenTaskLists();
    }

    void on_actionRemoveList_triggered()
    {
        RemoveTaskList(active_task_list_);
        UpdateDisplayOpenTaskLists();
    }

    void on_actionImportList_triggered(void)
    {
        if(!LoadTaskListFromFile()) return;
        UpdateDisplayOpenTaskLists();
    }

    void on_actionExportList_triggered(void)
    {
        SaveTaskListToFile(active_task_list_, TaskListSave::kExport);
    }

    void on_menuQuit_triggered(void)
    {
        PromptSaveTask();
        PromptSaveTaskList();
        this->close();
    }

    void on_lwOpenTaskLists_itemClicked(QListWidgetItem *item)
    {
        PromptSaveTask();
        PromptSaveTaskList();
        UpdateDisplayOpenTaskLists();
    }

    void on_lwTaskList_itemClicked(QListWidgetItem *item)
    {
        PromptSaveTask();
        UpdateDisplayActiveTask();
    }

    void on_pbCreateTask_clicked(void)
    {
        CreateTask();
        UpdateDisplayActiveTaskList();
    }

    void on_pbRemoveTask_clicked(void)
    {
        RemoveTask();
        UpdateDisplayActiveTaskList();
    }

    void on_pbLinkPrerequisite_clicked(void)
    {
        SelectPrereqToChange(TaskSelection::kAddPrerequisite);
    }

    void on_pbUnlinkPrerequisite_clicked(void)
    {
        SelectPrereqToChange(TaskSelection::kRemovePrerequisite);
    }

    void on_pbSaveChanges_clicked(void)
    {
        SaveActiveTask();
        UpdateDisplayActiveTaskList();
    }

    void on_pbToggleShowOpenTaskLists_clicked(void)
    {
        ui->lwOpenTaskLists->setVisible(!ui->lwOpenTaskLists->isVisible());
    }

    void on_teTitleTaskList_textChanged(void)
    {
        ValidateTaskListTitle();
    }

    void on_teTitle_textChanged(void)
    {
        ui->pbSaveChanges->setEnabled(true);
    }

    void on_teDescription_textChanged(void)
    {
        ui->pbSaveChanges->setEnabled(true);
    }

    void on_cbDeadline_stateChanged(int)
    {
        UpdateDisplayDateTimeCurrent(GetActiveTaskDeadline(), ui->cbDeadline, ui->dtDeadline);
        ui->pbSaveChanges->setEnabled(true);
    }

    void on_cbCompleted_stateChanged(int)
    {
        UpdateDisplayDateTimeCurrent(GetActiveTaskCompleted(), ui->cbCompleted, ui->dtCompleted);
        ui->pbSaveChanges->setEnabled(true);
    }

    void on_dtDeadline_dateTimeChanged(const QDateTime&)
    {
        ui->pbSaveChanges->setEnabled(true);
    }

    void on_dtCompleted_dateTimeChanged(const QDateTime&)
    {
        ui->pbSaveChanges->setEnabled(true);
    }

    void on_comboDependencies_editTextChanged(const QString&)
    {
        ui->pbSaveChanges->setEnabled(true);
    }

    void on_comboPrerequisites_editTextChanged(const QString&)
    {
        ui->pbSaveChanges->setEnabled(true);
    }

    void on_rbCurrent_clicked(void)
    {
        active_filter_ = TaskFilter::kCurrent;
        UpdateDisplayActiveTaskList();
    }

    void on_rbPending_clicked(void)
    {
        active_filter_ = TaskFilter::kPending;
        UpdateDisplayActiveTaskList();
    }

    void on_rbCompleted_clicked(void)
    {
        active_filter_ = TaskFilter::kCompleted;
        UpdateDisplayActiveTaskList();
    }

    void on_rbAll_clicked(void)
    {
        active_filter_ = TaskFilter::kAll;
        UpdateDisplayActiveTaskList();
    }

    void on_rbSortName_clicked(void)
    {
        active_sort_ = TaskSort::kName;
        UpdateDisplayActiveTaskList();
    }

    void on_rbSortDeadline_clicked(void)
    {
        active_sort_ = TaskSort::kDeadline;
        UpdateDisplayActiveTaskList();
    }

    void SlotChangePrerequisites(QStringList i_list, TaskSelection i_select)
    {
        ChangePrereq(i_list, i_select);
    }

};

#endif // MAINWINDOW_H
