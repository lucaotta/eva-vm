#pragma once

#include "evavalue.h"

#include <set>
class EvaCollector
{
public:
    EvaCollector() = default;

    // Use sets to eliminate duplicates
    void runGC(std::set<Traceable *> roots)
    {
        std::cout << "---- Before GC stats ----\n";
        Traceable::printStats();
        mark(roots);
        sweep();
        std::cout << "---- After GC stats ----\n";
        Traceable::printStats();
    }

private:
    void mark(std::set<Traceable *> roots)
    {
        std::vector<Traceable *> openNodes{roots.begin(), roots.end()};

        while (!openNodes.empty()) {
            auto node = openNodes.back();
            openNodes.pop_back();
            // TODO: handle cases where we have objects pointing to each other
            //            if (!node->marked) {
            //                openNodes.push_back(childNodes(node));
            //                node->marked = true;
            //            }
            node->marked = true;
        }
    }

    void sweep()
    {
        auto it = Traceable::objects.begin();
        while (it != Traceable::objects.end()) {
            auto t = *it;
            if (t->marked) {
                t->marked = false;
                ++it;
            } else {
                it = Traceable::objects.erase(it);
                delete t;
            }
        }
    }
};
