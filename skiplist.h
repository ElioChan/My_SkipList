#include <iostream>
#include <cstring>
#include <cmath>
#include <mutex>

#ifdef DEBUG
#define DBGprint(...) printf(__VA_ARGS__)
#else 
#define DBGprint(...) 
#endif

std::mutex mtx;

/*
    跳表中的节点 结构
    包含val next指针(forward)->level数组 节点的level 以及一些操作函数：get_key get_val set_val
    在Redis中还有backward指针 用于level 0 主要是为了提供逆序遍历 这里为了简化 没有设计

*/ 
template<typename K, typename  V>
class Node{
public:
    Node() {};
    Node(K key, V val, int );
    ~Node();
    K get_key() const;
    V get_val() const;
    void set_val(V);
    Node<K,V> **forward;
    int node_level;

private:
    K key;
    V val;
};

template <typename K, typename V>
Node<K, V>::Node(const K key, const V val, int level){
    this->key = key;
    this->val = val;
    this->node_level = level;

    this->forward = new Node<K, V> *[level + 1];
    memset(this->forward, 0, sizeof(Node<K, V>*)*(level + 1));
};

template<typename K, typename V>
Node<K, V>::~Node() {
    delete []forward;
};

template <typename K, typename V>
K Node<K, V>::get_key() const{
    return this->key;
}

template <typename K, typename V>
void Node<K, V>::set_val(V value) {
    this->val = value;
}

template <typename K, typename V>
V Node<K, V>::get_val() const{
    return this->val;
}


/*
    跳表
    本质上是多层链表 包含一个header指针 是每一层的头指针的数组
    以及一些操作函数
    有关控制跳表结构的参数 可以增加一个 random_level中的参数
*/
template <typename K, typename V>
class SkipList{
public:
    SkipList(int);
    ~SkipList();
    Node<K,V>* create_node(K, V, int);
    int size();
    int get_random_level();
    void display_list();
    bool search_element(K);
    int insert_element(K, V);
    void delete_element(K);

private:
    int _max_level;
    int _skip_list_level;  // 表示当前层数 从0开始 由于node节点中的forward数组初始化为level + 1 所以skip_list_level = 0表示有一层
    Node<K, V> *_header;
    int _element_count;

};

template <typename K, typename V>
SkipList<K, V>::SkipList(int max_level){
    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    K key;
    V value;
    this->_header = new Node<K, V> (key, value, _max_level);
}

template <typename K, typename V>
SkipList<K, V>::~SkipList() {
    delete _header; // TODO:可能存在内存泄漏 并未删除其他后续创造的节点 或许需要用户手动删除？？？
}

template <typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(K key, V val, int level){
    Node<K, V> *node = new Node<K, V> (key, val, level);
    return node;
}

template <typename K, typename V>
int SkipList<K, V>::size() {
    return this->_element_count;
}

template <typename K, typename V>
int SkipList<K, V>::get_random_level() {
    int level = 1;
    // TODO 这里的模2 可以更改为模3 模4 对比效率 以及 可以抽象为一个参数
    while(rand() % 2) {
        level++;
    }
    level = level < _max_level ? level : _max_level;
    return level;
}

template <typename K, typename V>
void SkipList<K, V>::display_list() {
    // 遍历每一层 显示每一层的数据
    // TODO 考虑改进display的美观性 目前每一层元素之间的间隔是相同的 考虑改进成 每个元素占一列
    std::cout << "********SkipList display!********" << std::endl;
    for(int i = 0; i < _skip_list_level; i++){
        Node<K, V> *current_node = this->_header->forward[i];
        std::cout << "Level " << i << ": ";
        while(current_node != nullptr){
            std::cout << current_node->get_key() << ": " << current_node->get_val() << ";    ";
            current_node = current_node->forward[i];
        }
        std::cout << std::endl;
    }
    return;
}

template <typename K, typename V> 
bool SkipList<K, V>::search_element(K key) {
    // 查找元素
    // 从最高层开始遍历元素 若cur->key < key则继续向右 进入下一层 直到最底层
    // DBGprint("Searching element ------------- ");
    // std::cout << "Searching element ------------- " << std::endl;

    if(this->_element_count == 0) {
        // DBGprint("Empty database now, Not Found Key!");
        // std::cout << "Empty database now, Not Found Key!" << std::endl;
        return false;
    }

    Node<K, V> *cur = this->_header;
    for(int i = _skip_list_level; i >= 0; i--) {
        while(cur->forward[i] && cur->forward[i]->get_key() < key) {
            cur = cur->forward[i];
        }
    }

    // 遍历至level 0中最后一个小于key 的元素 所以需要判断下一个元素是否等于查找的元素
    cur = cur->forward[0];
    if(cur && cur->get_key() == key) {
        // std::cout << "Found Key: " << cur->get_val() << ", Value: " << cur->get_val() <<  std::endl;
        return true;
    }

    // std::cout << "Not Found Key!" << std::endl;
    return false;
}

/*
    插入元素
    分为两部分 1 查找需要插入的位置 同时记录需要更改的节点 即每一行的最后一个小于插入节点key的元素
    2 创建节点 更改skip_list_level等信息 根据之前记录的节点信息 更改节点的forward指针 插入元素
    若已经存在元素 则插入失败 这一步可以在第一部分结束后判断
*/
template <typename K, typename V>
int SkipList<K, V>::insert_element(K key, V value) {
    
    mtx.lock();

    Node<K, V> *cur = this->_header;

    Node<K, V>* update[_max_level + 1];
    memset(update, 0, sizeof(Node<K,V>*)*(_max_level + 1));

    // 更新update数组 记录查找的过程
    for(int i = _skip_list_level; i >= 0; i--) {
        while(cur->forward[i] != nullptr && cur->forward[i]->get_key() < key) {
            cur = cur->forward[i];
        }
        update[i] = cur;
    }

    cur = cur->forward[0];
    // 判断是否已经存在key
    if(cur != nullptr && cur->get_key() == key) {
        // std::cout << "key: "<< key << " already exited!" << std::endl;
        mtx.unlock();
        return 1;
    }
    // 若不存在 则创建节点 并更改update数组中的forward指针 插入新节点
    else {
        // 创建新节点
        int level = get_random_level();
        if(level > _skip_list_level) {
            for(int i = _skip_list_level + 1; i < level + 1; i++) {
                update[i] = this->_header;
            }
            _skip_list_level = level;
        }
        Node<K, V>* node = create_node(key, value, level);
        // 更改forward指针
        for(int i = 0; i <= level; i++) {
            node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = node;
        }
        // std::cout << "Successfully inserted key: " << key << ", val: \"" << value << "\" into skiplist!" << std::endl;
        this->_element_count++;
    }

    mtx.unlock();
    return 0;
    
}

/*
    删除元素
    大致思路与插入相同 首先记录需要更改的节点  然后统一更改他们的forward指针信息
    更改skip_list_level的信息 可以遍历最高层 看header-forward是否为Null 若为空 则-1 
*/
template <typename K, typename V>
void SkipList<K, V>::delete_element(K key) {
    
    mtx.lock();

    Node<K, V>* cur = this->_header;

    Node<K, V>* update[_max_level + 1];

    for(int i = _skip_list_level; i >= 0; i--) {
        while(cur->forward[i] != nullptr && cur->forward[i]->get_key() < key) {
            cur = cur->forward[i];
        }
        update[i] = cur;
    }
    
    // 找到待删除节点
    cur = cur->forward[0];

    // 待删除节点存在
    if(cur != nullptr && cur->get_key() == key) {
        for(int i = 0; i < _skip_list_level; i++) {
            // 这里可以考虑在node类中提供一个访问层数level的函数接口
            if(update[i]->forward[i] != cur) break;

            update[i]->forward[i] = cur->forward[i];
        }
        // 更新skip_level信息
        while(_skip_list_level > 0 && _header->forward[_skip_list_level] == nullptr) {
            _skip_list_level--;
        }

        std::cout << "Successfully delete key: " << key << " !" << std::endl;
        delete cur;
        _element_count--;
    }

    mtx.unlock();
    return ;
}















