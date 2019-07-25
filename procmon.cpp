#include <sstream>
#include <string>
#include <map>
#include "procmon.h"

using std::map;
using std::string;

void GetDetails(string &details, map<string,string> &detail)
{
	while (details.length()>0)
	{
		auto colon = details.begin();
		string key;
		while (colon != details.end() && (*colon != ':' || (colon+1 != details.end() && *(colon+1) != ' ')))
		{
			key.push_back(*colon);
			colon++;
		}

		colon++;
		string value;
		while (colon != details.end() && (*colon != ':' || (colon + 1 != details.end() && *(colon + 1) != ' ')))
		{
			value.push_back(*colon);
			colon++;
		}
		
		if (colon != details.end())
		{
			auto delim = value.find_last_of(',');
			value = value.substr(0, delim);
		}
		else
		{

		}

		details = details.substr(key.length() + value.length() + 1);
		if (details.length()>0)
			details = details.substr(details.find_first_not_of(','));

		if (key.length())
			key = key.substr(0, key.find_last_not_of(' ') + 1);
		if (key.length())
			key = key.substr(key.find_first_not_of(' '));

		if (value.length())
			value = value.substr(0, value.find_last_not_of(' ') + 1);
		if (value.length())
			value = value.substr(value.find_first_not_of(' '));

		detail[key] = value;
	}
}

bool ParseCreate(ptree &node, std::string &path, map<string, string> &detail)
{
	auto OpenResult = detail.find("OpenResult");
	if (OpenResult != detail.end() && OpenResult->second == "Created")
	{
		node.put("Operation", "create");
		node.put("Path", path);
		return true;
	}

	return false;
}

bool ParseSetInformation(ptree &node, std::string &path, map<string, string> &detail)
{
	auto Type = detail.find("Type");

	if (Type != detail.end() && Type->second == "SetRenameInformationFile")
	{
		auto ReplaceIfExists = detail.find("ReplaceIfExists");

		node.put("Src", path);
		node.put("Dest", detail["FileName"]);

		if (ReplaceIfExists != detail.end() && ReplaceIfExists->second == "True")
		{
			//its a replace operation
			node.put("Operation", "replace");	
		}
		else
		{
			node.put("Operation", "rename");
		}

		return true;
	}

	if (Type != detail.end() && Type->second == "SetDispositionInformationFile")
	{
		auto Delete = detail.find("Delete");
		if (Delete != detail.end() && Delete->second == "True")
		{
			//its a delete operation.
			node.put("Operation", "delete");
			node.put("Path", path);
			return true;
		}
	}

	return false;
}

bool GetEventNodeFromProcmonNode(const ptree &node, ptree &ConvertedNode)
{
	auto Pid = node.get<unsigned long>("PID");
	auto Tid = node.get<unsigned long>("TID");
	auto operation = node.get<std::string>("Operation");
	auto Path = node.get<std::string>("Path");
	auto Detail = node.get<std::string>("Detail");

	ConvertedNode.put("PID", Pid);
	ConvertedNode.put("TID", Tid);

	std::map<string, string> DetailKeyValue;
	GetDetails(Detail, DetailKeyValue);

	if (operation == "IRP_MJ_READ")
	{
		ConvertedNode.put("Path", Path);
		ConvertedNode.put("Operation", "read");

		return true;
	}
	else if (operation == "IRP_MJ_WRITE")
	{
		ConvertedNode.put("Path", Path);
		ConvertedNode.put("Operation", "write");

		return true;
	}
	else if (operation == "IRP_MJ_CREATE")
	{
		return ParseCreate(ConvertedNode, Path, DetailKeyValue);
	}
	else if (operation == "IRP_MJ_SET_INFORMATION")
	{
		return ParseSetInformation(ConvertedNode, Path, DetailKeyValue);
	}
	else
	{
		return false;
	}
}

void Read_ProcmonXml(std::string XmlPath, ptree &Events)
{
	ptree ProcmonEvents;
	read_xml(XmlPath, ProcmonEvents);
	auto &TopLevel = Events.add("Events", "");
	
	ProcmonEvents = ProcmonEvents.get_child("procmon");
	for (auto &node : ProcmonEvents.get_child("eventlist"))
	{
		if (node.first != "event")
			continue;
		
		ptree ConvertedNode;
		if (GetEventNodeFromProcmonNode(node.second, ConvertedNode))
		{
			TopLevel.add_child("Event", ConvertedNode);
		}
	}
}