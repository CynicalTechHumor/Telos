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

#include "task.h"

// Constructors & Destructor

Task::Task(void)
{
    name_          = "Task";
    description_   = "";
    deadline_      = QDateTime();
    completed_     = QDateTime();
    prerequisites_ = std::vector<Task*>();
    dependencies_  = std::vector<Task*>();
}

Task::Task(QString i_name, QString i_description, QDateTime i_deadline, QDateTime i_completed)
{
    name_          = i_name;
    description_   = i_description;
    deadline_      = i_deadline;
    completed_     = i_completed;
    prerequisites_ = std::vector<Task*>();
    dependencies_  = std::vector<Task*>();
}

Task::~Task(void)
{
}

bool Task::AreTaskPrereqComplete(void)
{
    if (prerequisites_.empty()) return true;       // Return true if no prerequisites
    for(Task* i : prerequisites_)
        if (!(i->IsTaskComplete())) return false;  // Return false if any prerequisite is incomplete...
    return true;                                   // ...otherwise, return true
}

void Task::AddTaskPrereq(Task *i_task)
{
    if (!i_task) return;                // Do nothing if input is nullptr
    if (!prerequisites_.empty())
        for (Task *i : prerequisites_)
            if (i == i_task) return;    // Do nothing if input matches an existing prereq
    prerequisites_.push_back(i_task);   // Otherwise, add input to prereqs
}

void Task::AddTaskDepend(Task *i_task)
{
    if (!i_task) return;                // Do nothing if input is nullptr
    if (!dependencies_.empty())
        for (Task *i : dependencies_)
            if (i == i_task) return;    // Do nothing if input matches an existing depend
    dependencies_.push_back(i_task);    // Otherwise, add input to depends
}

void Task::RemoveTaskPrereq(Task *i_ptr)
{
    int index{GetPtrIndex(i_ptr, prerequisites_)};
    if (index == -1) return;                              // Do nothing if prereq isn't in list
    prerequisites_.erase(prerequisites_.begin() + index); // Otherwise, remove prereq
}

void Task::RemoveTaskDepend(Task *i_ptr)
{
    int index{GetPtrIndex(i_ptr, dependencies_)};
    if (index == -1) return;                              // Do nothing if depend isn't in list
    dependencies_.erase(dependencies_.begin() + index);   // Otherwise, remove depend
}

QStringList Task::GetTaskNames(std::vector<Task*> i_list)
{
    QStringList r_list;
    for (int i=0; i<i_list.size(); ++i) r_list.push_back(i_list[i]->GetTaskName());
    return r_list;
}

int Task::GetPtrIndex(Task* i_ptr, std::vector<Task*> i_list)
{
    int index{-1};
    for (int i{0}; i<i_list.size() && index == -1; ++i)
        if (i_list[i] == i_ptr)
            index = i;
    return index;
}

QStringList Task::SubtractTaskNames(QStringList in_1, QStringList in_2)
{
    QStringList out;
    QString current;
    bool match_found{false};

    // Iterate through first list - if an entry does not match any entry in second list, add it to output list
    while(!in_1.isEmpty())
    {
        current = in_1.front();
        for (int i{0}; i<in_2.size() && !match_found; ++i)
            if (current == in_2.at(i))
                match_found = true;
        if(!match_found)
            out.append(current);
        in_1.pop_front();
        match_found=false;
    }
    return out;
}

// Constructors & Destructor

TaskList::TaskList(void)
{
    name_ = "";
    list_ = std::vector<std::unique_ptr<Task>>();
}

TaskList::TaskList(QString input_name)
{
    name_ = input_name;
    list_ = std::vector<std::unique_ptr<Task>>();
}

TaskList::~TaskList(void)
{
}

std::vector<Task*> TaskList::GetAllTaskPtrsFromList(void)
{
    std::vector<Task*> list_ptrs;
    for (int i=0; i<list_.size(); ++i)
        list_ptrs.push_back(list_[i].get());
    return list_ptrs;
}

QStringList TaskList::GetAllTaskNamesFromList(void)
{
    QStringList list_names;
    for(int i=0; i<list_.size(); ++i)
        list_names.append(list_[i]->GetTaskName());
    return list_names;
}

int TaskList::GetIndexFromTaskList(QString i_name)
{
    int index{-1};
    for (int i=0; i<list_.size() && index == -1; ++i)
        if(list_[i]->GetTaskName()==i_name)
            index = i;
    return index;
}

int TaskList::GetIndexFromTaskList(Task* i_ptr)
{
    int index{-1};
    for (int i=0; i<list_.size() && index == -1; ++i)
        if(list_[i].get()==i_ptr)
            index = i;
    return index;
}

Task* TaskList::GetPtrFromTaskList(QString i_name)
{
    Task *o_ptr = nullptr;
    for (int i=0; i<list_.size(); ++i)
        if(list_[i]->GetTaskName()==i_name)
            o_ptr = list_[i].get();
    return o_ptr;
}

std::vector<Task*> TaskList::GetPtrsFromTaskList(QStringList i_list)
{
    std::vector<Task*> o;
    Task* temp;
    for(int i=0; i<i_list.size(); ++i)
    {
        temp = GetPtrFromTaskList(i_list.at(i));
        if (temp) o.push_back(temp);
    }
    return o;
}

bool TaskList::CheckDuplicateTaskName(QString i_name, Task *i_ptr)
{
    for(int i=0; i<list_.size(); ++i)
        if (list_[i]->GetTaskName() == i_name && list_[i].get() != i_ptr)
            return true;
    return false;
}

void TaskList::GetChainedPrereq(QStringList *o_list, QString i_name)
{
    // Check input name, throw logic exception if input is not valid
    Task* i_ptr{GetPtrFromTaskList(i_name)};
    if (i_name.isEmpty() || !i_ptr)
        throw std::logic_error("Invalid task name in prerequisite chain");

    // Add the current name to the list if not already present
    if (o_list->indexOf(i_name) == -1)
        o_list->push_back(i_name);

    // Recursively call prerequisites to find all chained prerequisites
    // Base case: no prerequisites to call
    std::vector<Task*> prereqs = i_ptr->GetTaskPrereq();
    for (Task* i : prereqs)
        this->GetChainedPrereq(o_list, i->GetTaskName());
}

void TaskList::GetChainedDepend(QStringList *o_list, QString i_name)
{
    // Check input name, throw exception if input is not valid
    Task* i_ptr{GetPtrFromTaskList(i_name)};
    if (i_name.isEmpty() || !i_ptr)
        throw std::logic_error("Invalid input name in dependency chain");

    // Add the current name to the list if not already present
    if (o_list->indexOf(i_name) == -1)
        o_list->push_back(i_name);

    // Recursively call prerequisites to find all chained dependents
    // Base case: no dependents to call
    std::vector<Task*> depend = i_ptr->GetTaskDepend();
    for (Task* i : depend)
        this->GetChainedDepend(o_list, i->GetTaskName());
}

void TaskList::SetTaskPrereqFromList(QString i_task, QStringList i_task_prereq)
{
    Task* task_ptr{GetPtrFromTaskList(i_task)};
    std::vector<Task*> task_prereq_ptrs = GetPtrsFromTaskList(i_task_prereq);
    for (Task* i : task_prereq_ptrs)
        task_ptr->AddTaskPrereq(i);
}

void TaskList::SetTaskDependFromList(QString i_task, QStringList i_task_depend)
{
    Task* task_ptr{GetPtrFromTaskList(i_task)};
    std::vector<Task*> task_depend_ptrs = GetPtrsFromTaskList(i_task_depend);
    for (Task* i : task_depend_ptrs)
        task_ptr->AddTaskDepend(i);
}

Task* TaskList::AddTaskToList(QString   i_name,
                              QString   i_description,
                              QDateTime i_deadline,
                              QDateTime i_completed)
{
    list_.push_back(std::make_unique<Task>(i_name, i_description, i_deadline, i_completed));
    return list_.back().get();
}

void TaskList::RemoveTaskFromList(QString i_name)
{
    int index{GetIndexFromTaskList(i_name)};
    if (index == -1) return;
    RemoveIndexFromList(index);
}

void TaskList::RemoveTaskFromList(Task* i_ptr)
{
    int index{GetIndexFromTaskList(i_ptr)};
    if (index == -1) return;
    RemoveIndexFromList(index);
}

void TaskList::RemoveIndexFromList(int index)
{
    // First, remove the task from the prereqs/depends of associated tasks
    std::vector<Task*> prereqs = list_[index]->GetTaskPrereq(),
                       depends = list_[index]->GetTaskDepend();
    for (int i=0; i<prereqs.size(); ++i)
        prereqs[i]->RemoveTaskDepend(list_[index].get());
    for (int i=0; i<depends.size(); ++i)
        depends[i]->RemoveTaskPrereq(list_[index].get());

    // Then, erase the task
    list_.erase(list_.begin() + index);
}
