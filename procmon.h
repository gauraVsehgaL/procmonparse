#include <iostream>
#include <boost\property_tree\xml_parser.hpp>
#include <boost\property_tree\ptree.hpp>

using boost::property_tree::ptree;

void Read_ProcmonXml(std::string XmlPath, ptree &Events);
