#ifndef XMLextractorHelpers_h__
#define XMLextractorHelpers_h__

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
namespace XMLhelper {
  template <typename T>
  std::vector<T> extractCutOffCondition1(rapidxml::xml_node<> *cutOffCon) {
    std::vector<T> ret;
    if (cutOffCon) {
      double A, B, C, D;
      auto node = cutOffCon->first_node("condition");

      while (node) {
        A = std::stod(node->first_attribute("A")->value());
        B = std::stod(node->first_attribute("B")->value());
        C = std::stod(node->first_attribute("C")->value());
        D = std::stod(node->first_attribute("D")->value());
        ret.emplace_back(A, B, C, D);
        node = node->next_sibling("condition");
      }
    }
    return ret;
  }
  template <typename XMLOBJECT, typename T>
  T getValue(XMLOBJECT *xmlnode, T defaultValue) {
    T ret = defaultValue;

    if (xmlnode) {
      std::stringstream ss(xmlnode->value());
      ss >> ret;
    }
    return ret;
  }
}

#endif // XMLextractorHelpers_h__
