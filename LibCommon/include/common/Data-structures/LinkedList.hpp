/*
Copyright (©) 2022-2025  Frosty515

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _LINKED_LIST_HPP
#define _LINKED_LIST_HPP

#include <cassert>
#include <cstdint>
#include <cstdio>

#include <functional>

#include "../spinlock.h"

namespace LinkedList {

    struct Node {
        Node* previous;
        uint64_t data;
        Node* next;
    };

    // Get length of the Linked list
    uint64_t length(Node* head);

    // Helper function that allocates a new node with the given data and NULL previous and next pointers.
    Node* newNode(uint64_t data);

    // Recursive function to insert a node in the list with given data and returns the head node
    void insertNode(Node*& head, uint64_t data);

    // Get a pointer to a node from its data
    Node* findNode(Node* head, uint64_t data);

    // Delete a node
    void deleteNode(Node*& head, uint64_t key);

    // Delete a specific node
    void deleteNode(Node*& head, Node* node);

    // print the Linked list
    void fprint(FILE* file, Node* head);

    void panic(const char* str); // a tiny function which just expands the PANIC macro. This is so PANIC can be called from the template class below.

    template<typename T>
    class SimpleLinkedList {
    public:
        SimpleLinkedList()
            : m_count(0), m_start(nullptr) {}
        ~SimpleLinkedList() {
            for (uint64_t i = 0; i < m_count; i++)
                remove((uint64_t)0);
        }

        void insert(const T* obj) {
            // if (findNode(m_start, (uint64_t)&obj) != nullptr)
            // 	return; // object already exists
            insertNode(m_start, (uint64_t)obj);
            m_count++;
        }
        void insertAt(uint64_t index, const T* obj) {
            if (index >= m_count)
                return;
            if (index == 0) {
                Node* node = newNode(reinterpret_cast<uint64_t>(obj));
                node->next = m_start;
                if (m_start != nullptr)
                    m_start->previous = node;
                m_start = node;
                m_count++;
                return;
            }
            Node* previous = nullptr;
            Node* temp = m_start;
            for (uint64_t i = 0; i < index; i++) {
                if (temp == nullptr) {
                    temp = previous;
                    break;
                }
                previous = temp;
                temp = temp->next;
            }
            Node* node = newNode(reinterpret_cast<uint64_t>(obj));
            // node needs to slot in between temp and temp->next
            if (temp == nullptr) {
                // empty list
                m_start = node;
                m_count++;
                return;
            }
            if (temp->next != nullptr) {
                temp->next->previous = node;
                node->next = temp->next;
            }
            temp->next = node;
            node->previous = temp;
            m_count++;
        }
        T* get(uint64_t index) const {
            if (index >= m_count)
                return nullptr;
            Node* temp = m_start;
            for (uint64_t i = 0; i < index; i++) {
                if (temp == nullptr)
                    return nullptr;
                temp = temp->next;
            }
            if (temp == nullptr)
                return nullptr;
            return (T*)(temp->data);
        }
        T* getNext(const T* obj) const {
            Node* temp = findNode(m_start, (uint64_t)obj);
            if (temp == nullptr)
                return nullptr;
            if (temp->next == nullptr)
                return nullptr;
            return (T*)(temp->next->data);
        }
        uint64_t getIndex(const T* obj) const {
            Node* temp = m_start;
            for (uint64_t i = 0; i < m_count; i++) {
                if (temp == nullptr)
                    return UINT64_MAX;
                if (temp->data == (uint64_t)obj)
                    return i;
                temp = temp->next;
            }
            return UINT64_MAX;
        }
        void remove(uint64_t index) {
            deleteNode(m_start, (uint64_t)get(index));
            m_count--;
        }
        void remove(const T* obj) {
            deleteNode(m_start, (uint64_t)obj);
            m_count--;
        }
        void rotateLeft() {
            if (m_count < 2)
                return; // not enough nodes to rotate
            Node* end = m_start;
            assert(end != nullptr);
            while (end->next != nullptr)
                end = end->next;
            assert(end != nullptr);
            end->next = m_start;
            m_start->previous = end;
            m_start = m_start->next;
            m_start->previous = nullptr;
            end->next->next = nullptr;
        }
        void rotateRight() {
            if (m_count < 2)
                return; // not enough nodes to rotate
            Node* end = m_start;
            assert(end != nullptr);
            while (end->next != nullptr)
                end = end->next;
            assert(end != nullptr);
            m_start->previous = end;
            end->next = m_start;
            end->previous = nullptr;
            m_start = end;
        }
        T* getHead() {
            if (m_start == nullptr)
                return nullptr;
            return (T*)(m_start->data);
        }
        void fprint(FILE* file) {
            fprintf(file, "LinkedList order: ");
            Node* node = m_start;
            for (uint64_t i = 0; i < m_count; i++) {
                if (node == nullptr)
                    break;
                fprintf(file, "%lx ", node->data);
                node = node->next;
            }
            fprintf(file, "\n");
        }

        [[nodiscard]] uint64_t getCount() const {
            return m_count;
        }

    private:
        uint64_t m_count;
        Node* m_start;
    };

    template<typename T>
    class RearInsertLinkedList {
    public:
        RearInsertLinkedList()
            : m_count(0), m_start(nullptr), m_end(nullptr), m_lock(SPINLOCK_DEFAULT_VALUE) {}
        ~RearInsertLinkedList() {
            for (uint64_t i = 0; i < m_count; i++)
                remove((uint64_t)0);
        }

        void insert(const T* obj) {
            Node* node = newNode((uint64_t)obj);
            if (m_count == 0) {
                m_start = node;
                m_end = node;
            } else {
                m_end->next = node;
                node->previous = m_end;
                m_end = node;
            }
            m_count++;
        }

        void insertAt(uint64_t index, const T* obj) {
            if (index >= m_count)
                return;
            Node* previous = nullptr;
            Node* temp = m_start;
            for (uint64_t i = 0; i < index; i++) {
                if (temp == nullptr) {
                    temp = previous;
                    break;
                }
                previous = temp;
                temp = temp->next;
            }
            Node* node = newNode(reinterpret_cast<uint64_t>(obj));
            // node needs to slot in between temp and temp->next
            if (temp == nullptr) {
                // empty list
                m_start = node;
                m_end = node;
                m_count++;
                return;
            }
            if (temp->next != nullptr) {
                temp->next->previous = node;
                node->next = temp->next;
            } else
                m_end = node;
            temp->next = node;
            node->previous = temp;
            m_count++;
        }

        T* get(uint64_t index) const {
            if (index >= m_count)
                return nullptr;
            Node* temp = m_start;
            for (uint64_t i = 0; i < index; i++) {
                if (temp == nullptr)
                    return nullptr;
                temp = temp->next;
            }
            if (temp == nullptr)
                return nullptr;
            return (T*)(temp->data);
        }

        uint64_t getIndex(const T* obj) const {
            Node* temp = m_start;
            for (uint64_t i = 0; i < m_count; i++) {
                if (temp == nullptr)
                    return UINT64_MAX;
                if (temp->data == (uint64_t)obj)
                    return i;
                temp = temp->next;
            }
            return UINT64_MAX;
        }

        void remove(uint64_t index) {
            deleteNode(m_start, (uint64_t)get(index));
            m_count--;
        }

        void remove(const T* obj) {
            deleteNode(m_start, (uint64_t)obj);
            m_count--;
        }

        void Enumerate(std::function<void(T* obj)> func) const {
            Node* temp = m_start;
            for (uint64_t i = 0; i < m_count; i++) {
                // if (temp == nullptr)
                // 	return;
                func((T*)(temp->data));
                temp = temp->next;
            }
        }

        void Enumerate(std::function<bool(T* obj, uint64_t index)> func, uint64_t start = 0) const {
            Node* temp = m_start;
            for (uint64_t i = 0; i < start; i++) {
                // if (temp == nullptr)
                // 	return;
                temp = temp->next;
            }
            for (uint64_t i = start; i < m_count; i++) {
                // if (temp == nullptr)
                // 	return;
                if (!func((T*)(temp->data), i))
                    return;
                temp = temp->next;
            }
        }

        void EnumerateReverse(std::function<bool(T* obj)> func) const {
            Node* temp = m_end;
            for (uint64_t i = 0; i < m_count; i++) {
                // if (temp == nullptr)
                // 	return;
                func((T*)(temp->data));
                temp = temp->previous;
            }
        }

        void EnumerateReverse(std::function<bool(T* obj, uint64_t index)> func, uint64_t start = 0) const {
            Node* temp = m_end;
            for (uint64_t i = 0; i < start; i++) {
                // if (temp == nullptr)
                // 	return;
                temp = temp->previous;
            }
            for (uint64_t i = start; i < m_count; i++) {
                // if (temp == nullptr)
                // 	return;
                if (!func((T*)(temp->data), i))
                    return;
                temp = temp->previous;
            }
        }

        [[nodiscard]] uint64_t getCount() const {
            return m_count;
        }

        void clear() {
            for (uint64_t i = 0; i < m_count; i++)
                deleteNode(m_start, m_start);
            m_count = 0;
            m_end = nullptr;
        }

        void lock() const {
            spinlock_acquire(&m_lock);
        }

        void unlock() const {
            spinlock_release(&m_lock);
        }

    private:
        uint64_t m_count;
        Node* m_start;
        Node* m_end;
        mutable spinlock_t m_lock;
    };

    template<typename T>
    class LockableLinkedList { // has a internal SimpleLinkedList and a spinlock. We do not lock automatically, so the user must lock the list before using it.
    public:
        LockableLinkedList()
            : m_list(), m_lock() {}
        ~LockableLinkedList() {
            spinlock_acquire(&m_lock);
            m_list.~SimpleLinkedList();
            spinlock_release(&m_lock);
        }

        void insert(const T* obj) {
            m_list.insert(obj);
        }
        T* get(uint64_t index) const {
            return m_list.get(index);
        }
        uint64_t getIndex(const T* obj) const {
            return m_list.getIndex(obj);
        }
        void remove(uint64_t index) {
            m_list.remove(index);
        }
        void remove(const T* obj) {
            m_list.remove(obj);
        }
        void rotateLeft() {
            m_list.rotateLeft();
        }
        void rotateRight() {
            m_list.rotateRight();
        }
        T* getHead() {
            return m_list.getHead();
        }
        void fprint(FILE* file) {
            m_list.fprint(file);
        }
        uint64_t getCount() const {
            return m_list.getCount();
        }

        void lock() const {
            spinlock_acquire(&m_lock);
        }
        void unlock() const {
            spinlock_release(&m_lock);
        }

    private:
        SimpleLinkedList<T> m_list;
        mutable spinlock_t m_lock;
    };

} // namespace LinkedList

extern bool operator==(LinkedList::Node left, LinkedList::Node right);

#endif /* _LINKED_LIST_HPP */
