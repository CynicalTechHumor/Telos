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

#ifndef TASK_H
#define TASK_H

#include <QDateTime>

// Task()
// Encapsulates all information about a task to be completed
class Task
{
public:

    // *************************
    // Constructors & Destructor
    // *************************

    Task();
    Task(QString i_name, QString i_description, QDateTime i_deadline, QDateTime i_completed);
    ~Task();

    // *********
    // Accessors
    // *********

    QString            GetTaskName        (void) { return name_;                }
    QString            GetTaskDescription (void) { return description_;         }
    QDateTime          GetTaskDeadline    (void) { return deadline_;            }
    QDateTime          GetTaskCompleted   (void) { return completed_;           }
    std::vector<Task*> GetTaskPrereq      (void) { return prerequisites_;       }
    std::vector<Task*> GetTaskDepend      (void) { return dependencies_;        }
    bool               IsTaskComplete     (void) { return completed_.isValid(); }

    bool AreTaskPrereqComplete(void);

    // ********
    // Mutators
    // ********

    void SetTaskName        (QString            input_string   ) { name_          = input_string;    }
    void SetTaskDescription (QString            input_string   ) { description_   = input_string;    }
    void SetTaskDeadline    (QDateTime          input_datetime ) { deadline_      = input_datetime;  }
    void SetTaskCompleted   (QDateTime          input_datetime ) { completed_     = input_datetime;  }
    void SetTaskPrereq      (std::vector<Task*> input_task_list) { prerequisites_ = input_task_list; }
    void SetTaskDepend      (std::vector<Task*> input_task_list) { dependencies_  = input_task_list; }

    // Add/remove task prereq/depend
    // Does nothing if input is invalid
    void AddTaskPrereq    (Task*);
    void AddTaskDepend    (Task*);
    void RemoveTaskPrereq (Task*);
    void RemoveTaskDepend (Task*);

    // ******
    // Static
    // ******

    // GetTaskNames()
    // Returns a string list of the names of all tasks in the input vector
    static QStringList GetTaskNames (std::vector<Task*>);

    // GetPtrIndex()
    // Returns the index of the input pointers location in the input vector
    // Returns -1 if the input pointer does not appear in the input vector
    static int GetPtrIndex(Task*, std::vector<Task*>);

    // SubtractTaskNames() - static
    // Remove strings from list 1 that match any string in list 2
    // Useful for removing tasks from a list which are not applicable to a specific function
    // i.e. when adding a new prerequisite, remove existing prerequisites/dependencies from the options
    static QStringList SubtractTaskNames(QStringList, QStringList);

protected:

    // Data
    QString            name_;
    QString            description_;
    QDateTime          deadline_;
    QDateTime          completed_;
    std::vector<Task*> prerequisites_;
    std::vector<Task*> dependencies_;

};

// TaskList()
// Encapsulates a list of tasks, and gives the list a unique name
// Also used for file structure - each .dat file corresponds to one task list
class TaskList
{

public:

    // *************************
    // Constructors & Destructor
    // *************************

    TaskList();
    TaskList(QString);
    ~TaskList();

    // *********
    // Accessors
    // *********

    QString GetTaskListName (void)  { return name_;          }
    int     GetTaskListSize (void)  { return list_.size();   }
    bool    IsTaskListEmpty (void)  { return list_.empty();  }
    Task*   operator[]      (int i) { return list_[i].get(); }

    // GetAllTaskPtrsFromList()
    // Get a vector of pointers to all Tasks currently in the list
    std::vector<Task*> GetAllTaskPtrsFromList(void);

    // GetAllTaskNamesFromList()
    // Get a string list of the names of all Tasks currently in the list
    QStringList GetAllTaskNamesFromList(void);

    // GetIndexFromTaskList()
    // Return the vector index of a specific task, identified by either pointer or name
    // Return -1 if the task is not in the list
    int GetIndexFromTaskList(QString);
    int GetIndexFromTaskList(Task*);

    // GetPtrFromTaskList(), GetPtrsFromTaskList
    // Return pointer (or a vector of pointers) to the task(s) identified by name
    // Return nullptr/empty vector if the task(s) is/are not in the list
    Task* GetPtrFromTaskList(QString i_name);
    std::vector<Task*> GetPtrsFromTaskList(QStringList i_list);

    // CheckDuplicateTaskName()
    // Checks list for any existing task with the input name
    // Also takes an input pointer identifying one task with that name to be ignored
    // Useful for preventing duplicate task names
    bool CheckDuplicateTaskName(QString, Task* = nullptr);

    // GetChainedPrereq(), GetChainedDepend()
    // Get all prerequisites/dependents for a given task in list, including all others in the chain
    // i.e. prerequisites of prerequisites, dependents of dependents
    void GetChainedPrereq(QStringList*, QString);
    void GetChainedDepend(QStringList*, QString);

    // ********
    // Mutators
    // ********

    void SetTaskListName        (QString i_name) { name_ = i_name; }
    void RemoveAllTasksFromList (void)           { list_.clear();  }

    // SetTaskPrereqFromList(), SetTaskDependFromList()
    // Remove the prerequistes/dependents (string list) from the given task (string)
    // Ignores items in list which are not prerequistes/dependents of the given task
    void SetTaskPrereqFromList(QString, QStringList);
    void SetTaskDependFromList(QString, QStringList);

    // AddTaskToList()
    // Creates a new task for the list, constructed using the input information
    Task* AddTaskToList(QString, QString = QString(), QDateTime = QDateTime(), QDateTime = QDateTime());

    // RemoveTaskFromList()
    // Removes task from the list, matched by either name or pointer
    void  RemoveTaskFromList(QString);
    void  RemoveTaskFromList(Task*);

protected:

    // Data
    QString                            name_;
    std::vector<std::unique_ptr<Task>> list_;

    // RemoveIndexFromList()
    // Removing a task from the list by index position in vector
    void RemoveIndexFromList(int index);

};

#endif // TASK_H
