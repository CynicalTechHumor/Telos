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
    if (!i_task) return;
    if (prerequisites_.end() != std::find(prerequisites_.begin(), prerequisites_.end(), i_task));
        prerequisites_.push_back(i_task);
}

void Task::AddTaskDepend(Task *i_task)
{
    if (!i_task) return;
    if (dependencies_.end() != std::find(dependencies_.begin(), dependencies_.end(), i_task));
        dependencies_.push_back(i_task);
}

void Task::RemoveTaskPrereq(Task *i_ptr)
{
    std::vector<Task*>::iterator my_task_iter = std::find(prerequisites_.begin(), prerequisites_.end(), i_ptr);
    if (my_task_iter != prerequisites_.end()) prerequisites_.erase(my_task_iter);
}

void Task::RemoveTaskDepend(Task *i_ptr)
{
    std::vector<Task*>::iterator my_task_iter = std::find(dependencies_.begin(), dependencies_.end(), i_ptr);
    if (my_task_iter != dependencies_.end()) dependencies_.erase(my_task_iter);
}

QStringList Task::GetTaskNames(std::vector<Task*> i_list)
{
    QStringList r_list;
    for (std::vector<Task*>::iterator i = i_list.begin(); i != i_list.end(); ++i) r_list.push_back((*i)->GetTaskName());
    return r_list;
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
    list_ = Task::PtrUniqueVector();
}

TaskList::TaskList(QString input_name)
{
    name_ = input_name;
    list_ = Task::PtrUniqueVector();
}

TaskList::~TaskList(void)
{
}

std::vector<Task*> TaskList::GetAllTaskPtrsFromList(void)
{
    Task::PtrVector list_ptrs;
    for (Task::PtrUniqueVectorIterate i = list_.begin(); i<list_.end(); ++i)
        list_ptrs.push_back((*i).get());
    return list_ptrs;
}

QStringList TaskList::GetAllTaskNamesFromList(void)
{
    QStringList list_names;
    for (Task::PtrUniqueVectorIterate i = list_.begin(); i<list_.end(); ++i)
        list_names.append((*i)->GetTaskName());
    return list_names;
}

Task::PtrVector TaskList::GetAllCompleted(void)
{
    Task::PtrVector list_ptrs;
    for (Task::PtrUniqueVectorIterate i = list_.begin(); i<list_.end(); ++i)
        if ((*i)->GetTaskCompleted().isValid())
            list_ptrs.push_back((*i).get());
    return list_ptrs;
}

Task* TaskList::GetPtrFromTaskList(QString i_name)
{
    Task *o_ptr = nullptr;
    for (Task::PtrUniqueVectorIterate i = list_.begin(); i<list_.end(); ++i)
        if((*i)->GetTaskName()==i_name)
            o_ptr = (*i).get();
    return o_ptr;
}

std::vector<Task*> TaskList::GetPtrsFromTaskList(QStringList i_list)
{
    Task::PtrVector o;
    Task* temp;
    for(int i=0; i<i_list.size(); ++i)
    {
        temp = GetPtrFromTaskList(i_list[i]);
        if (temp) o.push_back(temp);
    }
    return o;
}

bool TaskList::CheckDuplicateTaskName(QString i_name, Task *i_ptr)
{
    for (Task::PtrUniqueVectorIterate i = list_.begin(); i<list_.end(); ++i)
        if ((*i)->GetTaskName() == i_name && (*i).get() != i_ptr)
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
    Task::PtrVector prereqs = i_ptr->GetTaskPrereq();
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
    Task::PtrVector depend = i_ptr->GetTaskDepend();
    for (Task* i : depend)
        this->GetChainedDepend(o_list, i->GetTaskName());
}

void TaskList::GetCompleted(QStringList* o_list)
{
    for(Task::PtrUniqueVectorIterate i = list_.begin(); i != list_.end(); ++i)
        if ((*i)->GetTaskCompleted().isValid())
            o_list->push_back((*i)->GetTaskName());
}

void TaskList::SetTaskPrereqFromList(QString i_task, QStringList i_task_prereq)
{
    Task* task_ptr{GetPtrFromTaskList(i_task)};
    Task::PtrVector task_prereq_ptrs = GetPtrsFromTaskList(i_task_prereq);
    for (Task* i : task_prereq_ptrs)
        task_ptr->AddTaskPrereq(i);
}

void TaskList::SetTaskDependFromList(QString i_task, QStringList i_task_depend)
{
    Task* task_ptr{GetPtrFromTaskList(i_task)};
    Task::PtrVector task_depend_ptrs = GetPtrsFromTaskList(i_task_depend);
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
    Task* i_ptr = GetPtrFromTaskList(i_name);
    Task::PtrUniqueVectorIterate i = std::find_if(list_.begin(), list_.end(), [i_ptr](Task::PtrUnique& e) {return e.get() == i_ptr;});
    if (i != list_.end()) list_.erase(i);
}

void TaskList::RemoveTaskFromList(Task* i_ptr)
{
    Task::PtrUniqueVectorIterate i = std::find_if(list_.begin(), list_.end(), [i_ptr](Task::PtrUnique& e) {return e.get() == i_ptr;});
    if (i != list_.end()) list_.erase(i);
}

void TaskList::RemoveTasksFromList(Task::PtrVector i_list)
{
    for (Task* i : i_list)
        RemoveTaskFromList(i);
}
