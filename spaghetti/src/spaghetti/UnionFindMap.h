
#ifndef SPAGHETTI_UNION_FIND_MAP
#define SPAGHETTI_UNION_FIND_MAP

#include <unordered_map>
#include <vector>

template<typename T>
class UnionFindMap{
    std::unordered_map<T, T>      forest;

    T                             findHelper( typename std::unordered_map<T, T>::iterator );

    public:
    void                          merge(T const a, T const B);
    T                             find(T const a);
    std::vector<std::vector<T> >  getConnectedComponents();
};

template<typename T>
inline void UnionFindMap<T>::merge(T const a, T const b){
    forest[find(a)] = find(b);
}

template<typename T>
inline T UnionFindMap<T>::findHelper( typename std::unordered_map<T, T>::iterator it){
    if(it->first == it->second)
        return it->first;
    else{
        auto next_it = forest.find(it->second);
        T repr = findHelper(next_it);
        it->second = repr;
        return repr;
    }
}

template<typename T>
inline T UnionFindMap<T>::find(T const a){
    auto it = forest.find(a);
    if(it != forest.end()){
        return findHelper(it);
    }
    else{
        forest.emplace(a, a);
        return a;
    }
}

template<typename T>
inline std::vector<std::vector<T> > UnionFindMap<T>::getConnectedComponents(){
    std::unordered_map<T, std::vector<T> > components;
    for(std::pair<T, T> elt : forest){
        components[find(elt.first)].push_back(elt.first);
    }
    std::vector<std::vector<T> > ret;
    for(auto comp : components)
        ret.push_back(comp.second);
    return ret;
}

#endif

