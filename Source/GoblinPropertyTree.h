#ifndef GOBLIN_PROPERTY_TREE_H
#define GOBLIN_PROPERTY_TREE_H

#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>

namespace Goblin {
    class PropertyTree;
    using boost::property_tree::ptree;
    typedef std::vector<std::pair<std::string, PropertyTree> > PtreeList;

    class PropertyTree {
    public:
        PropertyTree(const ptree& pt);
        PropertyTree() {};
        bool read(const std::string& fileName);
        const PtreeList& getChildren() const;
        bool getChildren(const char* key, PtreeList* children) const;
        bool hasChild(const char* key) const;
        bool getChild(const char* key, PropertyTree* child) const;
        bool parseBool(const char* key, bool fallback = false) const;
        float parseFloat(const char* key, float fallback = 0.0f) const;
        int parseInt(const char* key, int fallback = 0) const;
        std::string parseString(const char* key, const char* fallback = "") const;
        std::vector<float> parseFloatArray(const char* key) const;

    private:
        ptree mPtree;
        PtreeList mChildren;
    };
}

#endif //GOBLIN_PROPERTY_TREE_H
