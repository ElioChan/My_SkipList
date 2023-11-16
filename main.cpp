#include <iostream>
#include "skiplist.h"

int main() {
    SkipList<int, std::string> sl(6);
    sl.insert_element(1, "First insert");
    sl.insert_element(22, "Second insert");
    sl.insert_element(3, "Third insert");
    sl.insert_element(14, "Forth insert");
    sl.insert_element(1, "Fifth insert");

    std::cout << "skiplist size: " <<  sl.size() << std::endl;
    sl.display_list();

    sl.search_element(3);
    sl.search_element(8);

    sl.delete_element(3);
    sl.display_list();
    sl.delete_element(15);

    return 0;
}