// vim:tabstop=4:shiftwidth=4:noexpandtab:textwidth=80
/*
 * $RCSfile$
 *
 * $Log$
 * Revision 1.1  2006/05/31 20:44:10  hockm0bm
 * * IConfigurationParser interface added
 * * StringConfigurationParser basic implemetation
 *
 *
 */
#ifndef _CONFPARSER_H_
#define _CONFPARSER_H_

#include<iostream>

namespace configuration
{

/** Interface for configuration parsers.
 * Defines methods needed for parsing configuration file. Template paramters
 * specifies concrete types for configuration entities which allways consist of
 * two parst key and value, where key determines meaning of value. Implementator
 * gives meaning to both of them.
 * <br>
 * Current unparsed key, value pair may be skipped by skip method.
 * <br>
 * Note that this interface doesn't say how format looks like or whether data
 * are read by lines or what so ever. One parse method call fills key and value
 * and moves to following one or returns with false if no more data are
 * available.
 */
template<typename KeyType=std::string, typename ValueType=std::string>
class IConfigurationParser
{
public:
	/** Empty virtual destructor.
	 */
	virtual ~IConfigurationParser(){}

	/** Skipps current key, value pair.
	 */
	virtual void skip()=0;

	/** Parse current key, value pair.
	 * @param key Reference where to put parsed key.
	 * @param value Reference where to put parsed value.
	 *
	 * Moves current parsing position after parsed data.
	 *
	 * @return true if parsing was successfull or false otherwise.
	 */
	virtual bool parse(KeyType & key, ValueType & value)=0;

	/** Checks whether we are on end of data.
	 *
	 * @return true if there is nothing more to read, false otherwise.
	 */
	virtual bool eod()=0;
};

/** Simple string parser.
 * Reads input stream by lines and split line according deliminer. Everything
 * before deliminers member is marked as key and everything after as value. 
 * <br>
 * Deliminers are specified as set of characters where each stands for
 * separator. 
 */
class StringConfigurationParser: public IConfigurationParser<std::string, std::string>
{
	/** Set of deliminers.
	 * Each character is one deliminer.
	 * Default value contains `:' character.
	 */
	std::string deliminerSet;

	/** Set of characters for comments.
	 * Default value contains `%', `#' characters.
	 */
	std::string commentsSet;

	/** Set of characters for blanks.
	 * Default value contains ` ', `\t' characters.
	 */
	std::string blankSet;

	/** Input stream with data.
	 */
	std::istream & stream;
	
	/** Maximum line length constant.
	 */
	static const size_t LINELENGTH=1024;
public:
	/** Initialization constructor.
	 * @param str Input stream with data.
	 *
	 * Initializes stream field with str and initializes *Set fields with
	 * default values.
	 */
	StringConfigurationParser(std::istream & str):stream(str)
	{
		deliminerSet=":";
		commentsSet="%#";
		blankSet=" \t";
	};

	/** Sets new commentsSet.
	 * @param set Set of characters.
	 *
	 * @return Current set value.
	 */
	std::string setCommentsSet(std::string set)
	{
		std::string old=commentsSet;

		commentsSet=set;

		return old;
	}

	/** Sets new blankSet.
	 * @param set Set of characters.
	 *
	 * @return Current set value.
	 */
	std::string setBlankSet(std::string set)
	{
		std::string old=blankSet;

		blankSet=set;

		return old;
	}

	/** Sets new deliminerSet.
	 * @param set Set of characters.
	 *
	 * @return Current set value.
	 */
	std::string setDeliminersSet(std::string set)
	{
		std::string old=deliminerSet;

		deliminerSet=set;

		return old;
	}

	/** Reads one line and throw it away.
	 */
	void skip()
	{
		// reads one line and forget it
		char buffer[LINELENGTH];
		stream.getline(buffer, LINELENGTH);
	}

	/** Parses valid line.
	 * Reads line from current stream position. Removes comments (everything
	 * behind membet of commentsSet character). Removes intials and trailing
	 * blanks (all characters from blankSet). If result data are empty, parses
	 * following line. Otherwise starts parsing.
	 * <br>
	 * Parsing process is very simple and just searches first occurence of
	 * character from deliminerSet. Everything before separator is used to
	 * initialize key parameter and everything behind to value parameter. If
	 * none from deliminerSet is found assumes that value is empty and key is
	 * whole parsed string.
	 * <br>
	 * If any error occures while reading or end of file occured, returns with
	 * false and key, value parameters are not filled.
	 * <br>
	 * Note that input stream can't contain 0 characters.
	 *
	 * @return true if line was parsed or false otherwise.
	 */
	bool parse(std::string & key, std::string & value)
	{
		char buffer[LINELENGTH];
		char *bufferStart=buffer;
		memset(buffer, '\0', sizeof(buffer));

		do
		{
			// proccess one line
			
			if(stream.eof())
				return false;
			
			stream.getline(buffer, LINELENGTH);
			if(stream.bad())
				// error during reading occured
				return false;

			// removes leading blanks
			for(size_t pos=0; pos<LINELENGTH; pos++)
			{
				if(blankSet.find(bufferStart))
					bufferStart++;
				else
					break;
			}
			
			// skips comments
			if(char * commentStart=strpbrk(bufferStart, commentsSet.c_str()))
				*bufferStart='\0';

			// skips trailing blanks
			for(char * end=bufferStart+strlen(bufferStart)-1; end>=bufferStart; end--)
			{
				if(blankSet.find(*end))
					*end='\0';
				else
					break;
			}
		}while(*bufferStart);

		// we have read one non empty line we can parse it to get key and value
		if(char * delim=strpbrk(bufferStart, deliminerSet.c_str()))
		{
			*delim='\0';
			key=bufferStart;
			value=delim+1;
		}else
			// no deliminer found so we initialize value as empty
			value="";

		return true;
	}

	/** Returns istream::eof().
	 */
	bool eod()
	{
		return stream.eof();
	}
};

} // namespace configuration
#endif