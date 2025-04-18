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

#ifndef _AVL_TREE_HPP
#define _AVL_TREE_HPP

#include <cstdint>

#include <Spinlock.hpp>

namespace AVLTree {

	struct Node {
		// actual data
		uint64_t key;
		uint64_t extraData;

		// Required extras
		Node* left;
		Node* right;
		int64_t height;
	};

	// Get height of the AVL tree
	int64_t height(Node* root);

	// Helper function that allocates a new node with the given key, extra data and NULL left and right pointers.
	Node* newNode(uint64_t key, uint64_t extraData);

	// A utility function to right rotate subtree.
	Node* rightRotate(Node* root);

	// A utility function to left rotate subtree.
	Node* leftRotate(Node* root);

	// Get Balance factor of node N.
	int64_t getBalance(Node* N);

	// Recursive function to insert a key in the subtree and returns the freshly inserted node.
	Node* insertNode(Node*& root, uint64_t key, uint64_t extraData);

	// Get a pointer to a node from its key
	Node* findNode(Node* root, uint64_t key);

	// Get a pointer to a node from its key or the key with the next highest value
	Node* findNodeOrHigher(Node* root, uint64_t key, uint64_t* actual_key = nullptr);

	// Get a pointer to a node from its key or the key with the next lowest value
	Node* findNodeOrLower(Node* root, uint64_t key, uint64_t* actual_key = nullptr);

	// Get the node with the lowest value in the tree
	Node* minValueNode(Node* root);

	// Delete a node
	void deleteNode(Node*& root, uint64_t key);

	// find Parent node of a key
	Node* getParent(Node* root, uint64_t key);

	template <typename K, typename D>
	class SimpleAVLTree {
	public:
		SimpleAVLTree() : m_root(nullptr) {}
		~SimpleAVLTree() {
			clear();
		}

		void insert(K key, D extraData) {
			insertNode(m_root, (uint64_t)key, (uint64_t)extraData);
		}

		void remove(K key) {
			deleteNode(m_root, (uint64_t)key);
		}

		D find(K key) const {
			Node* node = findNode(m_root, (uint64_t)key);
			if (node == nullptr)
				return (D)0;
			return (D)node->extraData;
		}

		D findOrHigher(K key, K* actual_key = nullptr) const {
			uint64_t actualKey;
			Node* node = findNodeOrHigher(m_root, (uint64_t)key, &actualKey);
			if (node == nullptr)
				return (D)0;
			if (actual_key != nullptr)
				*actual_key = (K)actualKey;
			return (D)node->extraData;
		}

		D findOrLower(K key, K* actual_key = nullptr) const {
			uint64_t actualKey;
			Node* node = findNodeOrLower(m_root, (uint64_t)key, &actualKey);
			if (node == nullptr)
				return (D)0;
			if (actual_key != nullptr)
				*actual_key = (K)actualKey;
			return (D)node->extraData;
		}

		void clear() {
			while (m_root != nullptr)
				deleteNode(m_root, m_root->key);
		}

		[[nodiscard]] Node* GetRoot() const {
			return m_root;
		}

		Node*& GetRootRef() {
			return m_root;
		}

	private:
		Node* m_root;
		bool m_vmm{};
	};

	template <typename K, typename D>
	class LockableAVLTree {
	public:
		explicit LockableAVLTree(bool vmm = false) : m_list(vmm) {
			spinlock_init(&m_lock);
		}

		~LockableAVLTree() {
			m_list.clear();
		}

		void insert(K key, D extraData) {
			m_list.insert(key, extraData);
		}

		void remove(K key) {
			m_list.remove(key);
		}

		D find(K key) const {
			return m_list.find(key);
		}

		D findOrHigher(K key, K* actual_key = nullptr) const {
			return m_list.findOrHigher(key, actual_key);
		}

		D findOrLower(K key, K* actual_key = nullptr) const {
			return m_list.findOrLower(key, actual_key);
		}

		void clear() {
			m_list.clear();
		}

		Node* GetRoot() const {
			return m_list.GetRoot();
		}

		Node*& GetRootRef() {
			return m_list.GetRootRef();
		}

		void lock() {
			spinlock_acquire(&m_lock);
		}

		void unlock() {
			spinlock_release(&m_lock);
		}

		// TEMPORARY
		void EnumerateInternal(Node* root, void(*callback)(K, D, void*), void* data) {
			if (root != nullptr) {
				if (root->left != nullptr)
					EnumerateInternal(root->left, callback, data);
				callback((K)root->key, (D)root->extraData, data);
				if (root->right != nullptr)
					EnumerateInternal(root->right, callback, data);
			}
		}

		// TEMPORARY
		void Enumerate(void(*callback)(K, D, void*), void* data) {
			EnumerateInternal(m_list.GetRoot(), callback, data);
		}
		
	private:
		SimpleAVLTree<K, D> m_list;
		spinlock_t m_lock{};
	};
}

#endif /* _AVL_TREE_HPP */
