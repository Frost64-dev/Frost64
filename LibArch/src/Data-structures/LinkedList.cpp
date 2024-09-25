/*
Copyright (©) 2022-2024  Frosty515

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

#include <Data-structures/LinkedList.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <util.h>

namespace InsEncoding {

	bool operator==(LinkedList::Node left, LinkedList::Node right) {
		return ((left.data == right.data) && (left.next == right.next) && (left.previous == right.previous));
	}

	namespace LinkedList {

		uint64_t length(Node* head) {
			if (head == nullptr)
				return 0;
			
			Node* current = head;
			uint64_t count = 0;
			while (true) {
				count++;
				if (current->next == nullptr) break;
				current = current->next;
			}

			current = nullptr; // protects the node that current points to from potential deletion

			return count;
		}

		Node* newNode(uint64_t data) {
			Node* node = new Node();

			if (node == nullptr)
				return nullptr; // protects against page faults

			node->data = data;
			node->previous = nullptr;
			node->next = nullptr;
			return node;
		}

		void insertNode(Node*& head, uint64_t data) {
			// check if head is NULL
			if (head == nullptr) {
				head = newNode(data);
				return;
			}

			// move to last node
			Node* current = head;
			while (true) {
				if (current->next == nullptr) break;
				current = current->next;
			}

			// get new node and set last node's next to it
			current->next = newNode(data);

			// update newly created node's previous to the last node
			current->next->previous = current;

			// clear the value of current to protect the node it is pointing to from possible deletion
			current = nullptr;
		}

		Node* findNode(Node* head, uint64_t data) {
			Node* current = head;
			while (current != nullptr) {
				if (current->data == data) return current;
				current = current->next;
			}
			return nullptr;
		}

		void deleteNode(Node*& head, uint64_t data) {
			Node* temp = head;
			if (temp != nullptr && temp->data == data) {
				head = temp->next;
				if (head != nullptr) {
					head->previous = nullptr;
					if (head->next != nullptr)
						head->next->previous = head;
				}
				delete temp;
				return;
			}
			while (temp->data != data) {
				if (temp->next == nullptr)
					return;
				temp = temp->next;
			}
			if (temp == nullptr)
				return;
			if (temp->next != nullptr)
				temp->next->previous = temp->previous;
			if (temp->previous != nullptr)
				temp->previous->next = temp->next;
			delete temp;
		}

		void deleteNode(Node*& head, Node* node) {
			if (node == nullptr || head == nullptr)
				return;
			if (node->next != nullptr)
				node->next->previous = node->previous;
			if (node->previous != nullptr)
				node->previous->next = node->next;
			if (head == node)
				head = node->next;
			delete node;
		}

		void fprint(FILE* file, Node* head) {
			fprintf(file, "Linked list order: ");

			Node* current = head;
			while (current != nullptr) {
				fprintf(file, " %lu ", current->data);
				current = current->next;
			}

			fprintf(file, "\n");

			// clear the value of current to protect the node it is pointing to from possible deletion
			current = nullptr;
		}

		void panic(const char* str) {
			printf("LinkedList::panic: %s\n", str);
			exit(1);
		}

	}

}
