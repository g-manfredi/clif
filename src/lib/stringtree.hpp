#ifndef _STRINGTREE_H
#define _STRINGTREE_H

#include <tuple>
#include <iostream>
#include <ostream>

/*
 c++14
 
template<typename TupleType, typename FunctionType>
void for_each(TupleType&&, FunctionType
            , std::integral_constant<size_t, std::tuple_size<typename std::remove_reference<TupleType>::type >::value>) {}

template<std::size_t I, typename TupleType, typename FunctionType
       , typename = typename std::enable_if<I!=std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type >
void for_each(TupleType&& t, FunctionType f, std::integral_constant<size_t, I>)
{
    f(std::get<I>(t));
    for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, I + 1>());
}

template<typename TupleType, typename FunctionType>
void for_each(TupleType&& t, FunctionType f)
{
    for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, 0>());
}


template<typename...TS> void printtuple(std::tuple<TS...> t)
{
  for_each(t, [](const auto &x) { if (x) std::cout << *x; });
}*/

template<typename TupleType>
void for_each_print(TupleType&&, std::integral_constant<size_t, std::tuple_size<typename std::remove_reference<TupleType>::type >::value>) {}

template<std::size_t I, typename TupleType, typename = typename std::enable_if<I!=std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type >
void for_each_print(TupleType&& t, std::integral_constant<size_t, I>)
{
    if (std::get<I>(t))
      std::cout << *(std::get<I>(t));
    for_each_print(std::forward<TupleType>(t), std::integral_constant<size_t, I + 1>());
}

template<typename TupleType>
void printtuple(TupleType&& t)
{
    for_each_print(std::forward<TupleType>(t), std::integral_constant<size_t, 0>());
}


template<typename... TS> class StringTree {
public:
  StringTree() {};
  StringTree(std::string name, TS...args);
  
  void print(int depth = 0);
  
  void add(std::string str, TS...args, char delim);
  int childCount();
  StringTree *operator[](int idx);
  
  std::pair<std::string, void*> *search(std::string str, char delim);
  
  std::pair<std::string, std::tuple<TS...>> val;
  std::vector<StringTree> childs;
};

template<typename... TS> StringTree<TS...>::StringTree(std::string name, TS ... args)
{
  val.first = name;
  val.second = std::tuple<TS...>(args...);
}

template<typename... TS> void StringTree<TS...>::print(int depth)
{
  const char *spacebuf = "                                                            ";
  /*for(int i=0;i<depth;i++)
    printf("   ");*/
  if (depth < 60/3) {
    std::cout.write(spacebuf, depth*3) << val.first;
    int rest = 60-depth*3-val.first.length();
    if (rest > 0)
      std::cout.write(spacebuf, rest);
  }
  else
    std::cout << " [...] " << val.first;
  printtuple(val.second);
  printf("\n");
  for(uint i=0;i<childs.size();i++)
    childs[i].print(depth+1);
}

template<typename... TS> void StringTree<TS...>::add(std::string str, TS...args, char delim)
{
  uint found = str.find(delim);
  std::string name = str.substr(0, found);
      
  for(uint i=0;i<childs.size();i++)
    if (!name.compare(childs[i].val.first)) {
      if (found < str.length()-1) {
        childs[i].add(str.substr(found+1), args..., delim);
        return;
      }
      else {
        printf("FIXME StringTree: handle existing elements in add!\n");
        return;
      }
    } 
  
  if (found < str.length()-1) {
    //FIXME different type?!
    childs.push_back(StringTree<TS...>(name,NULL));
    childs.back().add(str.substr(found+1), args..., delim);
  }
  else
    childs.push_back(StringTree(name,args...));
  
}
  
template<typename... TS> std::pair<std::string, void*> *StringTree<TS...>::search(std::string str, char delim)
{
  int found = str.find(delim);
  std::string name = str.substr(0, found);
  
  std::cout << "search: " << str << ":" << name << std::endl;
  
  for(int i=0;i<childs.size();i++)
    if (!name.compare(childs[i].val.first)) {
      if (found < str.length()-1) {
        return childs[i].search(str.substr(found+1), delim);
      }
      else
        return &val;
    } 
  
  std::cout << "not found: " << str << ":" << name << std::endl;
  return NULL;
}


template<typename... TS> StringTree<TS...> *StringTree<TS...>::operator[](int idx)
{
  return &childs[idx];
}

template<typename... TS> int StringTree<TS...>::childCount()
{
  return childs.size();
}

#endif