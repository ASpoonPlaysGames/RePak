#include <iostream>
#include <fstream>

class CKeyValues
{
	// THIS PARSER DOESNT SUPPORT:
	// - Multiple values for a key
	// - Multiple instances of a key
	// With that being said, it should be fine for a material?

public:
	/// <summary>
	/// Parses KeyValues from a specified path
	/// </summary>
	/// <param name="filePath">The path to the file to be parsed</param>
	inline void ParseFile(std::filesystem::path filePath) { ParseFile(filePath.string()); }

	/// <summary>
	/// Parses KeyValues from a specified path
	/// </summary>
	/// <param name="filePath">The path to the file to be parsed</param>
	void ParseFile(std::string filePath);
	
	/// <summary>
	/// Parses KeyValues from a stream
	/// </summary>
	/// <param name="stream">The stream to parse</param>
	void ParseStream(std::ifstream& stream);

	/// <summary>
	/// Checks if a Key exists
	/// </summary>
	/// <param name="key">The key to search for</param>
	/// <returns>true if the key is found, false if the key isn't found</returns>
	bool Contains(std::string key);

	/// <summary>
	/// Checks if a Key's value is itself a collection of KeyValues
	/// </summary>
	/// <param name="key">The key to check</param>
	/// <returns>true if the key is a collection of KeyValues, false otherwise</returns>
	bool IsKeyValues(std::string key);

	/// <summary>
	/// Gets a string value from a key
	/// </summary>
	/// <param name="key">The key to get the value of</param>
	/// <returns>The value found at the key</returns>
	std::string GetValue(std::string key);

	/// <summary>
	/// Gets the collection of KeyValues for a key
	/// </summary>
	/// <param name="key">The key to get the value of</param>
	/// <returns>The KeyValues found at the key</returns>
	CKeyValues* GetKeyValues(std::string key);

	~CKeyValues();
private:
	
	/// <summary>
	/// Stores the nested KeyValues for keys
	/// </summary>
	std::unordered_map<std::string, CKeyValues*> nestedKeyValues = {};
	/// <summary>
	/// Stores the raw Values for keys
	/// </summary>
	std::unordered_map<std::string, std::string> KeyValuePairs = {};
};

enum class ParseStatus
{
	None,
	InString,
	InValue, // this is a "value" meaning like 0 or something, not a Value like a KeyValue
	InComment
};