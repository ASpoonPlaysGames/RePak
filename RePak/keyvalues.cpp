#include "pch.h";
#include "keyvalues.h";

void CKeyValues::ParseFile(std::string filePath)
{
	// create the stream
	std::ifstream stream(filePath);
	if (!stream)
	{
		Error("Failed to open file '%s'", filePath);
	}
	stream.seekg(0, std::ios::beg);
	// pass the stream to ParseStream for actual parsing
	ParseStream(stream);
	// close the stream
	stream.close();
}

void CKeyValues::ParseStream(std::ifstream& stream)
{
	
	CKeyValues* value = nullptr;
	
	// track the current status for avoiding parsing comments and stuff wrong
	ParseStatus currentStatus = ParseStatus::None;

	// track the current char and the previous char
	char curChar;
	char prevChar = '\0';

	// track the current and previous strings that will eventually be added to the maps as key value pairs
	std::string curStr;
	std::string prevStr;

	bool isValue = false;

	bool shouldBreak = false;

	// loop through the entire file
	while (stream.get(curChar) && !shouldBreak)
	{
		
		switch (currentStatus)
		{
		case ParseStatus::InComment:
			// if we are in a comment, do nothing until the end of a line
			if (curChar == '\n')
			{
				// reset the currentStatus
				currentStatus = ParseStatus::None;
			}
			break;
		case ParseStatus::InValue:
			[[fallthrough]];
		case ParseStatus::InString:
			// if we are in a string, add the char to the current string (unless the char signifies the end of the string)
			if ((curChar == '"' && currentStatus == ParseStatus::InString) || (std::isspace(curChar) && currentStatus == ParseStatus::InValue))
			{
				// reset the currentStatus
				currentStatus = ParseStatus::None;
				if (isValue)
				{
					// if we just ended a value, then prevStr is the key, and curStr is the value
					this->KeyValuePairs.insert(std::make_pair(prevStr, curStr));
					isValue = false; 
				}
				else
				{
					isValue = true;
				}
				// save the current string for later
				prevStr = curStr;
				// clear current string
				curStr = "";
			}
			else
			{
				// add to curStr
				curStr += curChar;
			}
			break;
		case ParseStatus::None:
			if (std::isspace(curChar))
			{
				continue;
			}
			switch (curChar)
			{
			case '/':
				// if both curChar and prevChar are /, then we have just started a comment line
				if (prevChar == '/')
				{
					currentStatus = ParseStatus::InComment;
				}
				break;
			case '{':
				// make a new CKeyValues and parse it
				value = new CKeyValues();
				value->ParseStream(stream);
				this->nestedKeyValues.insert(std::make_pair(prevStr, value));
				break;
			case '}':
				// we have reached the end of our KeyValues, break from the loop early
				shouldBreak = true;
				break;
			case '"':
				currentStatus = ParseStatus::InString;
				break;
			default:
				// add char to string early because this char is needed (its not ")
				curStr += curChar;
				currentStatus = ParseStatus::InValue;
				break;
			}
			break;
		}

		// update prevChar
		prevChar = curChar;
	}
}


bool CKeyValues::Contains(std::string key)
{
	return this->KeyValuePairs.find(key) != this->KeyValuePairs.end() || this->nestedKeyValues.find(key) != this->nestedKeyValues.end();
}

bool CKeyValues::IsKeyValues(std::string key)
{
	return this->nestedKeyValues.find(key) != this->nestedKeyValues.end();
}

std::string CKeyValues::GetValue(std::string key)
{
	return this->KeyValuePairs.find(key)->second;
}

CKeyValues* CKeyValues::GetKeyValues(std::string key)
{
	return this->nestedKeyValues.find(key)->second;
}
